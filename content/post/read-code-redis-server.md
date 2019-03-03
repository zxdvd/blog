+++
title = "代码阅读: redis-server"
date = "2018-04-22"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-server"
Authors = "Xudong"
+++

server.c是main入口函数所在文件，里面有很多全局初始化，command table，cron等的处理.

#### shared 全局变量
有很多数值/字符串是非常常用的，比如`\r\n`, `0`, `1`, error_info等，将这些提前初始化好，在用的时候
可以循环共享使用，不需要频繁的malloc/free(避免系统调用开销).

    struct sharedObjectsStruct shared;

#### shared object的初始化
void createSharedObjects(void)
    int j;

    shared.crlf = createObject(OBJ_STRING,sdsnew("\r\n"));
    shared.ok = createObject(OBJ_STRING,sdsnew("+OK\r\n"));
    shared.err = createObject(OBJ_STRING,sdsnew("-ERR\r\n"));
    shared.emptybulk = createObject(OBJ_STRING,sdsnew("$0\r\n\r\n"));
    shared.czero = createObject(OBJ_STRING,sdsnew(":0\r\n"));
    shared.cone = createObject(OBJ_STRING,sdsnew(":1\r\n"));
    shared.cnegone = createObject(OBJ_STRING,sdsnew(":-1\r\n"));
    shared.nullbulk = createObject(OBJ_STRING,sdsnew("$-1\r\n"));
    shared.nullmultibulk = createObject(OBJ_STRING,sdsnew("*-1\r\n"));
    shared.emptymultibulk = createObject(OBJ_STRING,sdsnew("*0\r\n"));
    shared.pong = createObject(OBJ_STRING,sdsnew("+PONG\r\n"));
    shared.queued = createObject(OBJ_STRING,sdsnew("+QUEUED\r\n"));
    shared.emptyscan = createObject(OBJ_STRING,sdsnew("*2\r\n$1\r\n0\r\n*0\r\n"));
    // error info: wrongtypeerr, nokeyerr, syntaxerr, sameobjecterr, outofrangeerr, noscripterr, loadingerr
    // slowscripterr, masterdownerr, bgsaveerr, roslaveerr, noautherr, oomerr, execaborterr
    // noreplicaserr, busykeyerr
    shared.space = createObject(OBJ_STRING,sdsnew(" "));
    shared.colon = createObject(OBJ_STRING,sdsnew(":"));
    shared.plus = createObject(OBJ_STRING,sdsnew("+"));

    // 生成若干个select dbId的AOF指令缓存(并不是所有的id都有)
    for (j = 0; j < PROTO_SHARED_SELECT_CMDS; j++) {
        char dictid_str[64];
        int dictid_len;

        dictid_len = ll2string(dictid_str,sizeof(dictid_str),j);
        shared.select[j] = createObject(OBJ_STRING,
            sdscatprintf(sdsempty(),
                "*2\r\n$6\r\nSELECT\r\n$%d\r\n%s\r\n",
                dictid_len, dictid_str));
    }
    shared.messagebulk = createStringObject("$7\r\nmessage\r\n",13);
    shared.pmessagebulk = createStringObject("$8\r\npmessage\r\n",14);
    shared.subscribebulk = createStringObject("$9\r\nsubscribe\r\n",15);
    shared.unsubscribebulk = createStringObject("$11\r\nunsubscribe\r\n",18);
    shared.psubscribebulk = createStringObject("$10\r\npsubscribe\r\n",17);
    shared.punsubscribebulk = createStringObject("$12\r\npunsubscribe\r\n",19);
    shared.del = createStringObject("DEL",3);
    shared.unlink = createStringObject("UNLINK",6);
    shared.rpop = createStringObject("RPOP",4);
    shared.lpop = createStringObject("LPOP",4);
    shared.lpush = createStringObject("LPUSH",5);
    // 将从0到9999的数字缓存起来
    for (j = 0; j < OBJ_SHARED_INTEGERS; j++) {
        shared.integers[j] =
            makeObjectShared(createObject(OBJ_STRING,(void*)(long)j));
        shared.integers[j]->encoding = OBJ_ENCODING_INT;
    }
    for (j = 0; j < OBJ_SHARED_BULKHDR_LEN; j++) {
        shared.mbulkhdr[j] = createObject(OBJ_STRING,
            sdscatprintf(sdsempty(),"*%d\r\n",j));
        shared.bulkhdr[j] = createObject(OBJ_STRING,
            sdscatprintf(sdsempty(),"$%d\r\n",j));
    }
    shared.minstring = sdsnew("minstring");
    shared.maxstring = sdsnew("maxstring");

在initServer里会调用createStringObjects()来初始化.

#### 全局变量R_Zero, R_PosInf, R_NegInf, R_Nan
这几个都是double类型的全局变量，在initServerConfig里面初始化的

    R_Zero = 0.0;
    R_PosInf = 1.0/R_Zero;
    R_NegInf = -1.0/R_Zero;
    R_Nan = R_Zero/R_Zero;
这里面这么做的左右是将这几个明确化，因为redis的aof和rdb都是需要将内存
数据序列号到文件的.
所以将他们的值明确化就方便序列化和反序列化了(根据IEEE745至少nan的值是不明确的)

#### redis command table
所有commands的列表，可以根据名字获取对应的函数了.
第一个参数是liternal name，第二个是函数指针.
第三个表示函数的参数个数，如果是负数，比如-N表示参数个数>=N.
第四个参数是命令的flags的字符表示.
第五个是命令的flags的bitwise表示(初始为0).
第六个是获取参数里key的函数指针(常规方法已经不足以获取key)
第七个是参数里第一个key的index
第八个是参数里最后一个key的index
第九个是key与key直接的步进
第十个是微秒数
第十一个是调用次数统计

flags的各个字符的意义:
w: write; r: read; m: 可能增加内存; a: admin; p: pub/sub
f: 强制同步(忽略server.dirty); s: 该命令不能在lua script里执行
R: 随机命令(命令结果具有不确定性, 比如SPOP, RANDOMKEY);
S: 在lua里调用的时候将返回的数组排序(结果是确定的);
l: 在loading阶段允许执行的命令;
M: 结果不输出到monitor client;
F: O(1)或者O(log(N))的命令，没有delay，很快, 那些可能触发del的不是

    struct redisCommand redisCommandTable[] = {
        {"get",getCommand,2,"rF",0,NULL,1,1,1,0,0},
        {"set",setCommand,-3,"wm",0,NULL,1,1,1,0,0},
        {"setnx",setnxCommand,3,"wmF",0,NULL,1,1,1,0,0},
        {"del",delCommand,-2,"w",0,NULL,1,-1,1,0,0},
        {"brpoplpush",brpoplpushCommand,4,"wms",0,NULL,1,2,1,0,0},
        {"bgsave",bgsaveCommand,-1,"a",0,NULL,0,0,0,0,0},
    }
上面是一小部分的命令, get的flags 'rF'表示只读操作以及O(1)执行. set的'wm'是写操作并且会增加内存, -3表示参数大于等于3个(EX, PX, NX等是可选参数).

void populateCommandTable(void)这个函数算是redisCommandTable的初始化函数吧，上面的table里第五个参数是flags的
bitwise表达，但是都是0，populateCommandTable这个函数根据字符表达的值把它们的bitwise表达给初始化好了.
然后把所有的命令都保存在server.commands, server.orig_commands这两个dict里面.
orig_commands是原始的不会改变，而commands是会变的，比如rename.

command table相关的一些函数

    // 根据sds字符串来获取command
    struct redisCommand *lookupCommand(sds name)
    // 根据常规字符串来获取command
    struct redisCommand *lookupCommandByCString(char *s)
    // 先从commands里找，找不到就从orig_commands里找
    struct redisCommand *lookupCommandOrOriginal(sds name)


#### void serverLogRaw(int level, const char *msg)
log处理函数之一，根据server.logfile是否设置来确定输出到stdout还是server.logfile.
根据level确定是否需要输出以及是否需要pid, timestamp之类的其它信息.
最后根据server.syslog_enabled来确定是否需要同时输出到syslog.

#### void serverLog(int level, const char *fmt, ...)
用的最多的log函数，调用vsnprintf输出.

#### void serverLogFromHandler(int level, const char *msg)
这个是专门给signal handler用的，因为printf等stdio函数是不可重入的, 不能在signal handler里用,
这里用write系统调用直接写文件的方式来log.

#### dictType以及相关的函数的实现
看dict的实现那里会发现createDict的时候需要一个dictType结构，里面全是函数指针，用来
实现hash, dup, compare, match, destructor等功能.
server.c里实现了很多不同的这些方法并定义了很多需要用到的dictType.

    // dict的value为list的时候的destructor
    void dictListDestructor(void *privdata, void *val)
    // sds key的比较方法，相同返回1，不相同返回0
    int dictSdsKeyCompare(void *privdata, const void *key1, const void *key2)
    // sds key的忽略大小写比较方法，这里跟上面的dictSdsKeyCompare实现是不同的
    // 这里直接调用strcasecmp只能比较字符串，上面的是进行binary比较(memcmp)
    // 这里应该是根据实际的使用场景来确定这么比较的
    int dictSdsKeyCaseCompare(void *privdata, const void *key1, const void *key2)
    // object的destructor，减少引用计数，让bio的线程去做lazyfree
    void dictObjectDestructor(void *privdata, void *val)
    // sds是直接free的
    void dictSdsDestructor(void *privdata, void *val)
    // robj包装的key的比较函数, 调用了dictSdsKeyCompare
    int dictObjKeyCompare(void *privdata, const void *key1,
    // dictObjHash, dictSdsHash都是调用dictGenHashFunction处理的
    unsigned int dictObjHash(const void *key)
    unsigned int dictSdsHash(const void *key)
    // 这里是一个忽略大小写的hash函数
    unsigned int dictSdsCaseHash(const void *key)
    // 这个是key包装成robj的时候的compare和hash实现
    int dictEncObjKeyCompare(void *privdata, const void *key1, const void *key2)
    unsigned int dictEncObjHash(const void *key)

后面定义了很多不同的dictType.

#### hash table扩容和rehash

    // 这个是判断是否需要扩容(size > 4 && fill < 10%)
    int htNeedsResize(dict *dict)
    // 尝试resize一个db的dict和expires
    void tryResizeHashTables(int dbid)
    // 对一个db的dict和expires都执行1ms的rehash操作
    int incrementallyRehash(int dbid)
    // 如果有rdb或者aof进程在的话，就禁止rehash (rehash会导致大量的内存页的copy)
    void updateDictResizePolicy(void)

#### metrics
server.inst_metric保存了三组metric，每组可以保存16个sample值，通过这些可以大致了解server当前状况

    void trackInstantaneousMetric(int metric, long long current_reading)
    // 获取某一组metric的平均指标数值
    long long getInstantaneousMetric(int metric)

### crons
各种定期执行的cron处理.

#### client cron
下面这个是处理clienttimeout的情况

    int clientsCronHandleTimeout(client *c, mstime_t now_ms) {
        time_t now = now_ms/1000;

        // 如果server设置了maxidletime(对应config里的timeout, 默认是0，初始化也是0)
        // 并且client不是slave/master, 也没有处在BLPOP/BRPOP或者pubsub状态时
        // 然后idle时间超过了maxidletime, 那么就将该client free掉
        if (server.maxidletime &&
            !(c->flags & CLIENT_SLAVE) &&
            !(c->flags & CLIENT_MASTER) &&
            !(c->flags & CLIENT_BLOCKED) &&
            !(c->flags & CLIENT_PUBSUB) &&
            (now - c->lastinteraction > server.maxidletime))
        {
            serverLog(LL_VERBOSE,"Closing idle client");
            freeClient(c);
            return 1;
        } else if (c->flags & CLIENT_BLOCKED) {
            // 对于BLPOP/BRPOP，是有timeout设置的，如果不为0，则需要检查是否timeout了
            if (c->bpop.timeout != 0 && c->bpop.timeout < now_ms) {
                replyToBlockedClientTimedOut(c);
                unblockClient(c);
            } else if (server.cluster_enabled) {
                /* Cluster: handle unblock & redirect of clients blocked
                 * into keys no longer served by this server. */
                if (clusterRedirectBlockedClientIfNeeded(c))
                    unblockClient(c);
            }
        }
        return 0;
    }

下面这个是尝试从c->querybuf(sds字符串, 有额外预留空间的配置)里回收部分内存的cron

    int clientsCronResizeQueryBuffer(client *c)

#### void clientsCron(void) (有删减不重要部分)

    // 因为redis是每hz执行一次cron操作，这里将所有clients除以hz，目的是降低clientsCron的处理频率，每个client每秒钟只处理一次
    int numclients = listLength(server.clients);
    int iterations = numclients/server.hz;
    mstime_t now = mstime();

    while(listLength(server.clients) && iterations--) {
        client *c;
        listNode *head;
        // TODO
        listRotate(server.clients);
        head = listFirst(server.clients);
        c = listNodeValue(head);
        // 每个client都调用的处理函数
        if (clientsCronHandleTimeout(c,now)) continue;
        if (clientsCronResizeQueryBuffer(c)) continue;
    }
}

#### void databasesCron(void)
上面的是clients的cron处理，这个是db的cron处理.

    // 对于master，需要定期检查过期的key并做处理
    // slave并不需要，它们只需要等待master的del指令来处理过期的key
    if (server.active_expire_enabled && server.masterhost == NULL)
        activeExpireCycle(ACTIVE_EXPIRE_CYCLE_SLOW);
    // 如果没有rdb或者aof进程的时候(减少内存copy)，进行rehash操作
    if (server.rdb_child_pid == -1 && server.aof_child_pid == -1) {
        // 每次尝试默认处理CRON_DBS_PER_CALL个(16个)db, 因此需要全局变量来保存进度
        static unsigned int resize_db = 0;
        static unsigned int rehash_db = 0;
        int dbs_per_call = CRON_DBS_PER_CALL;
        int j;
        if (dbs_per_call > server.dbnum) dbs_per_call = server.dbnum;
        for (j = 0; j < dbs_per_call; j++) {
            tryResizeHashTables(resize_db % server.dbnum);
            resize_db++;
        }
        if (server.activerehashing) {
            // 如果某个db进行了rehash操作，work_done返回1，并且退出循环
            // 也就是每个周期最多真正的进行的rehashing只有一个db
            for (j = 0; j < dbs_per_call; j++) {
                int work_done = incrementallyRehash(rehash_db % server.dbnum);
                rehash_db++;
                if (work_done) {
                    break;
                }
            }
        }
    }

### int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientData)
整个redis server的cron的入口函数(几个参数至少目前都没有使用).
先介绍下`run_with_period`这个宏，定义如下

    #define run_with_period(_ms_) if ((_ms_ <= 1000/server.hz) || !(server.cronloops%((_ms_)/(1000/server.hz))))

因为redis有hz的配置，默认是10，表示每秒执行10次cron，也就是100ms一次，但是这是个配置项，如果想实现每隔多少ms执行什么就需要上面的这个宏.
server.cronloops每个cron周期都会增加1，是个计数器.

    // TODO
    if (server.watchdog_period) watchdogScheduleSignal(server.watchdog_period);

    updateCachedTime();                 // 更新系统时间

    // 每隔100ms更新metric的采样信息，包括命令数, 网络input/output字节数
    run_with_period(100) {
        trackInstantaneousMetric(STATS_METRIC_COMMAND,server.stat_numcommands);
        trackInstantaneousMetric(STATS_METRIC_NET_INPUT,
                server.stat_net_input_bytes);
        trackInstantaneousMetric(STATS_METRIC_NET_OUTPUT,
                server.stat_net_output_bytes);
    }

    server.lruclock = getLRUClock();

    // 更新peak_memory
    if (zmalloc_used_memory() > server.stat_peak_memory)
        server.stat_peak_memory = zmalloc_used_memory();

    server.resident_set_size = zmalloc_get_rss();

    // 处理shutdown，signal handler那里只是记录下需要shutdown，这里是执行
    if (server.shutdown_asap) {
        if (prepareForShutdown(SHUTDOWN_NOFLAGS) == C_OK) exit(0);
        serverLog(LL_WARNING,"SIGTERM received but errors trying to shut down the server, check the logs for more information");
        server.shutdown_asap = 0;
    }

    // (如果开启了verbose log) 每5s记录下每个非空db里面的dict size/used/expire keys的大小
    run_with_period(5000) {
        for (j = 0; j < server.dbnum; j++) {
            long long size, used, vkeys;

            size = dictSlots(server.db[j].dict);
            used = dictSize(server.db[j].dict);
            vkeys = dictSize(server.db[j].expires);
            if (used || vkeys) {
                serverLog(LL_VERBOSE,"DB %d: %lld keys (%lld volatile) in %lld slots HT.",j,used,vkeys,size);
            }
        }
    }
    if (!server.sentinel_mode) {
        run_with_period(5000) {
            serverLog(LL_VERBOSE,
                "%lu clients connected (%lu slaves), %zu bytes in use",
                listLength(server.clients)-listLength(server.slaves),
                listLength(server.slaves),
                zmalloc_used_memory());
        }
    }

    // client和database的cron在这里调用
    clientsCron();
    databasesCron();

    // 如果aof_rewrite_scheduled被标记了并且当前没有rdb/aof进程，就进行后台的aof操作
    if (server.rdb_child_pid == -1 && server.aof_child_pid == -1 &&
        server.aof_rewrite_scheduled)
    {
        rewriteAppendOnlyFileBackground();
    }

    // 如果有rdb/aof进程，如果wait获取到某个进程的退出信息，就执行对应的
    // 退出后的收尾善后工作
    if (server.rdb_child_pid != -1 || server.aof_child_pid != -1 ||
        ldbPendingChildren())
    {
        int statloc;
        pid_t pid;

        if ((pid = wait3(&statloc,WNOHANG,NULL)) != 0) {
            int exitcode = WEXITSTATUS(statloc);
            int bysignal = 0;
            // 盘点是不是由signal触发的进程退出，如果是获取对应的signal
            if (WIFSIGNALED(statloc)) bysignal = WTERMSIG(statloc);

            if (pid == -1) {
                serverLog(LL_WARNING,"wait3() returned an error: %s. "
                    "rdb_child_pid = %d, aof_child_pid = %d",
                    strerror(errno),
                    (int) server.rdb_child_pid,
                    (int) server.aof_child_pid);
            } else if (pid == server.rdb_child_pid) {
                backgroundSaveDoneHandler(exitcode,bysignal);
            } else if (pid == server.aof_child_pid) {
                backgroundRewriteDoneHandler(exitcode,bysignal);
            } else {
                if (!ldbRemoveChild(pid)) {
                    serverLog(LL_WARNING,
                        "Warning, detected child with unmatched pid: %ld",
                        (long)pid);
                }
            }
            // 通常在rdb/aof进程启动时为了减少内存页copy会disable rehash
            // 那么如果rdb/aof进程结束了，这里需要再次更新下策略
            updateDictResizePolicy();
        }
    } else {
         // redis的save可以同时有很多个配置比如100 10;500 40
         // 就是每100s有10个变化或者每500s有40个变化就save
         // 那么这里就需要遍历这些save配置来一个个的判断并处理
         // 如果任何一个条件满足就执行save然后退出循环
         for (j = 0; j < server.saveparamslen; j++) {
            struct saveparam *sp = server.saveparams+j;
            if (server.dirty >= sp->changes &&
                server.unixtime-server.lastsave > sp->seconds &&
                (server.unixtime-server.lastbgsave_try >
                 CONFIG_BGSAVE_RETRY_DELAY ||
                 server.lastbgsave_status == C_OK))
            {
                serverLog(LL_NOTICE,"%d changes in %d seconds. Saving...",
                    sp->changes, (int)sp->seconds);
                rdbSaveBackground(server.rdb_filename);
                break;
            }
         }

         // TODO
         if (server.rdb_child_pid == -1 &&
             server.aof_child_pid == -1 &&
             server.aof_rewrite_perc &&
             server.aof_current_size > server.aof_rewrite_min_size)
         {
            long long base = server.aof_rewrite_base_size ?
                            server.aof_rewrite_base_size : 1;
            long long growth = (server.aof_current_size*100/base) - 100;
            if (growth >= server.aof_rewrite_perc) {
                serverLog(LL_NOTICE,"Starting automatic rewriting of AOF on %lld%% growth",growth);
                rewriteAppendOnlyFileBackground();
            }
         }
    }


    if (server.aof_flush_postponed_start) flushAppendOnlyFile(0);
    run_with_period(1000) {
        if (server.aof_last_write_status == C_ERR)
            flushAppendOnlyFile(0);
    }
    freeClientsInAsyncFreeQueue();

    clientsArePaused(); /* Don't check return value, just use the side effect.*/

    run_with_period(1000) replicationCron();
    run_with_period(100) {
        if (server.cluster_enabled) clusterCron();
    }
    run_with_period(100) {
        if (server.sentinel_mode) sentinelTimer();
    }
    run_with_period(1000) {
        migrateCloseTimedoutSockets();
    }

    // 处理异步的bgsave请求(通过server.rdb_bgsave_scheduled标记)
    if (server.rdb_child_pid == -1 && server.aof_child_pid == -1 &&
        server.rdb_bgsave_scheduled &&
        (server.unixtime-server.lastbgsave_try > CONFIG_BGSAVE_RETRY_DELAY ||
         server.lastbgsave_status == C_OK))
    {
        if (rdbSaveBackground(server.rdb_filename) == C_OK)
            server.rdb_bgsave_scheduled = 0;
    }
    server.cronloops++;
    return 1000/server.hz;


#### void beforeSleep(struct aeEventLoop *eventLoop)
这个是每次进去eventloop前执行的，跟cron类似

    if (server.cluster_enabled) clusterBeforeSleep();
    // 如果是master，就执行一个fast expire处理
    if (server.active_expire_enabled && server.masterhost == NULL)
        activeExpireCycle(ACTIVE_EXPIRE_CYCLE_FAST);

    // TODO
    if (server.get_ack_from_slaves) {
        robj *argv[3];
        argv[0] = createStringObject("REPLCONF",8);
        argv[1] = createStringObject("GETACK",6);
        argv[2] = createStringObject("*",1); /* Not used argument. */
        replicationFeedSlaves(server.slaves, server.slaveseldb, argv, 3);
        decrRefCount(argv[0]);
        decrRefCount(argv[1]);
        decrRefCount(argv[2]);
        server.get_ack_from_slaves = 0;
    }
    if (listLength(server.clients_waiting_acks))
        processClientsWaitingReplicas();

    if (listLength(server.unblocked_clients))
        processUnblockedClients();
    flushAppendOnlyFile(0);
    handleClientsWithPendingWrites();



### 时间相关的
server.unixtime保存的是时间戳，精确到秒，server.mstime保存的是毫秒数(0-999).
这个时间也是定期更新的，对于一些不需要非常精确时间的地方，用这个来获取时间比用系统调用要快多了.

    void updateCachedTime(void) {
        server.unixtime = time(NULL);
        server.mstime = mstime();
    }

#### void propagate(struct redisCommand *cmd, int dbid, robj **argv, int argc,
               int flags)
根据flags同步命令到aof, slave.

    if (server.aof_state != AOF_OFF && flags & PROPAGATE_AOF)
        feedAppendOnlyFile(cmd,dbid,argv,argc);
    if (flags & PROPAGATE_REPL)
        replicationFeedSlaves(server.slaves,dbid,argv,argc);


#### int processCommand(client *c)
命令的处理

    // 处理quit命令
    if (!strcasecmp(c->argv[0]->ptr,"quit")) {
        addReply(c,shared.ok);
        c->flags |= CLIENT_CLOSE_AFTER_REPLY;
        return C_ERR;
    }
    // 处理找不到命令或者命令参数数量不对的情况
    c->cmd = c->lastcmd = lookupCommand(c->argv[0]->ptr);
    if (!c->cmd) {
        flagTransaction(c);
        addReplyErrorFormat(c,"unknown command '%s'",
            (char*)c->argv[0]->ptr);
        return C_OK;
    } else if ((c->cmd->arity > 0 && c->cmd->arity != c->argc) ||
               (c->argc < -c->cmd->arity)) {
        flagTransaction(c);
        addReplyErrorFormat(c,"wrong number of arguments for '%s' command",
            c->cmd->name);
        return C_OK;
    }

    // 如果server配置了密码, 则除AUTH之外的所有命令都需要认证
    if (server.requirepass && !c->authenticated && c->cmd->proc != authCommand)
    {
        flagTransaction(c); addReply(c,shared.noautherr); return C_OK;
    }

    // TODO
    if (server.cluster_enabled &&
        !(c->flags & CLIENT_MASTER) &&
        !(c->flags & CLIENT_LUA &&
          server.lua_caller->flags & CLIENT_MASTER) &&
        !(c->cmd->getkeys_proc == NULL && c->cmd->firstkey == 0 &&
          c->cmd->proc != execCommand))
    {
        int hashslot;
        int error_code;
        clusterNode *n = getNodeByQuery(c,c->cmd,c->argv,c->argc,
                                        &hashslot,&error_code);
        if (n == NULL || n != server.cluster->myself) {
            if (c->cmd->proc == execCommand) {
                discardTransaction(c);
            } else {
                flagTransaction(c);
            }
            clusterRedirectClient(c,n,hashslot,error_code);
            return C_OK;
        }
    }

    // 如果有最大内存限制需要特别处理
    if (server.maxmemory) {
        int retval = freeMemoryIfNeeded();
        // 在free内存的过程中是有可能导致当前client被free掉了，需要检查
        if (server.current_client == NULL) return C_ERR;
        if ((c->cmd->flags & CMD_DENYOOM) && retval == C_ERR) {
            flagTransaction(c); addReply(c, shared.oomerr); return C_OK;
        }
    }

    /* Don't accept write commands if there are problems persisting on disk
     * and if this is a master instance. */
     // 如果是master，且write类型的命令，然后aof或者rdb存盘失败的情况下,报错
    if (((server.stop_writes_on_bgsave_err &&
          server.saveparamslen > 0 &&
          server.lastbgsave_status == C_ERR) ||
          server.aof_last_write_status == C_ERR) &&
        server.masterhost == NULL &&
        (c->cmd->flags & CMD_WRITE ||
         c->cmd->proc == pingCommand))
    {
        flagTransaction(c);
        if (server.aof_last_write_status == C_OK)
            addReply(c, shared.bgsaveerr);
        else
            addReplySds(c,
                sdscatprintf(sdsempty(),
                "-MISCONF Errors writing to the AOF file: %s\r\n",
                strerror(server.aof_last_write_errno)));
        return C_OK;
    }

    // 如果正常工作的slaves数量少于min-slaves-to-write这个配置，不接受写操作命令
    if (server.masterhost == NULL &&
        server.repl_min_slaves_to_write &&
        server.repl_min_slaves_max_lag &&
        c->cmd->flags & CMD_WRITE &&
        server.repl_good_slaves_count < server.repl_min_slaves_to_write)
    {
        flagTransaction(c); addReply(c, shared.noreplicaserr); return C_OK;
    }

    // slave仅接受master作为client的命令(来同步数据), 其它情况都不接受写命令
    if (server.masterhost && server.repl_slave_ro &&
        !(c->flags & CLIENT_MASTER) &&
        c->cmd->flags & CMD_WRITE)
    {
        addReply(c, shared.roslaveerr); return C_OK;
    }

    if (c->flags & CLIENT_PUBSUB &&
        c->cmd->proc != pingCommand &&
        c->cmd->proc != subscribeCommand &&
        c->cmd->proc != unsubscribeCommand &&
        c->cmd->proc != psubscribeCommand &&
        c->cmd->proc != punsubscribeCommand) {
        addReplyError(c,"only (P)SUBSCRIBE / (P)UNSUBSCRIBE / PING / QUIT allowed in this context");
        return C_OK;
    }

    /* Only allow INFO and SLAVEOF when slave-serve-stale-data is no and
     * we are a slave with a broken link with master. */
    if (server.masterhost && server.repl_state != REPL_STATE_CONNECTED &&
        server.repl_serve_stale_data == 0 &&
        !(c->cmd->flags & CMD_STALE))
    {
        flagTransaction(c); addReply(c, shared.masterdownerr); return C_OK;
    }

    // 只有有CMD_LOADING标志的命令才能在server loading期间执行
    if (server.loading && !(c->cmd->flags & CMD_LOADING)) {
        addReply(c, shared.loadingerr); return C_OK;
    }

    /* Lua script too slow? Only allow a limited number of commands. */
    if (server.lua_timedout &&
          c->cmd->proc != authCommand &&
          c->cmd->proc != replconfCommand &&
        !(c->cmd->proc == shutdownCommand &&
          c->argc == 2 &&
          tolower(((char*)c->argv[1]->ptr)[0]) == 'n') &&
        !(c->cmd->proc == scriptCommand &&
          c->argc == 2 &&
          tolower(((char*)c->argv[1]->ptr)[0]) == 'k'))
    {
        flagTransaction(c);
        addReply(c, shared.slowscripterr);
        return C_OK;
    }

    // 命令执行
    if (c->flags & CLIENT_MULTI &&
        c->cmd->proc != execCommand && c->cmd->proc != discardCommand &&
        c->cmd->proc != multiCommand && c->cmd->proc != watchCommand)
    {
        queueMultiCommand(c);
        addReply(c,shared.queued);
    } else {
        call(c,CMD_CALL_FULL);
        c->woff = server.master_repl_offset;
        if (listLength(server.ready_keys))
            handleClientsBlockedOnLists();
    }
    return C_OK;


### shutdown
关闭所有的listening sockets(包括cluster, unix socket处理)

    void closeListeningSockets(int unlink_unix_socket) {
        int j;
        for (j = 0; j < server.ipfd_count; j++) close(server.ipfd[j]);
        if (server.sofd != -1) close(server.sofd);
        if (server.cluster_enabled)
            for (j = 0; j < server.cfd_count; j++) close(server.cfd[j]);
        if (unlink_unix_socket && server.unixsocket) {
            serverLog(LL_NOTICE,"Removing the unix socket file.");
            unlink(server.unixsocket);
        }
    }

#### int time_independent_strcmp(char *a, char *b)
这个函数是一个比较字符串的函数，它的特点是运行时间跟a,b无关，是个常量，这个用处是在密码比较的地方(authCommand), 提高安全性，
避免通过运行时间来猜解密码的情况.

### 一些命令的处理

    // AUTH命令的处理(密码连接)
    void authCommand(client *c)
    // ping命令的处理，可以有一个参数, 在pubsub模式跟常规模式处理是不一样的
    void pingCommand(client *c)
    // echo命令的处理
    void echoCommand(client *c)
    // 返回系统时间，秒数和微秒数
    void timeCommand(client *c)

#### void addReplyCommand(client *c, struct redisCommand *cmd)
这个函数是command指令的helper函数，输出某个命令的command table信息，包括flags
变成语义化的字符串(CMD_WRITE->write, CMD_READONLY->readonly), 以及
firstkey, lastkey, keystep信息.

#### void commandCommand(client *c)
处理command命令，command命令有多种形式
不带任何参数的时候，用上面的addReplyCommand来返回server.commands里面的每一个命令的详细信息.
command info <cmd>用addReplyCommand来返回某一个具体的命令的信息.
command count返回server.commands的个数.
command getkeys xx xx xx xx xx返回后面的命令里面的所有的key(按照顺序).

#### void bytesToHuman(char *s, unsigned long long n)
一个helper函数将n字节用human readable的方式表示，比如1024 -> 1K, 1024*1024 -> 1M.

#### sds genRedisInfoString(char *section)
info命令的处理, info命令可以接受一个参数section, 共有default, all, server, clients, memory, persistence, stats, replication, cpu, commandstats,
cluster, keyspace这些section.
all表示全部，default是默认参数(不包含commandstats)

#### int linuxOvercommitMemoryValue(void)
从/proc/sys/vm/overcommit_memory的值

#### 创建pidfile并写入pid的处理
void createPidFile(void) {
    /* If pidfile requested, but no pidfile defined, use
     * default pidfile path */
    if (!server.pidfile) server.pidfile = zstrdup(CONFIG_DEFAULT_PID_FILE);
    /* Try to write the pid file in a best-effort way. */
    FILE *fp = fopen(server.pidfile,"w");
    if (fp) {
        fprintf(fp,"%d\n",(int)getpid());
        fclose(fp);
    }
}

#### daemonize
void daemonize(void) {
    int fd;
    if (fork() != 0) exit(0); /* parent exits */
    setsid(); /* create a new session */

    /* Every output goes to /dev/null. If Redis is daemonized but
     * the 'logfile' is set to 'stdout' in the configuration file
     * it will not log at all. */
    if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) close(fd);
    }
}

#### static void sigShutdownHandler(int sig)
停止服务的信号处理, 第一次收到信号时会执行graceful的停止，第二次收到就快速退出了.

    char *msg;
    // 如果第二次收到信号就打log，删除rdb文件，然后退出(错误码1表示非正常退出)
    if (server.shutdown_asap && sig == SIGINT) {
        serverLogFromHandler(LL_WARNING, "You insist... exiting now.");
        rdbRemoveTempFile(getpid());
        exit(1);
    } else if (server.loading) {
        exit(0);
    }
    serverLogFromHandler(LL_WARNING, msg);
    // 第一次收到停止信号时标记一下，server会在后面来处理这个信号
    server.shutdown_asap = 1;

#### void setupSignalHandlers(void)
信号初始化和设置信号处理函数

    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = sigShutdownHandler;
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT, &act, NULL);

#### void loadDataFromDisk(void)
如果aof开启优先从aof文件恢复db数据，否则考虑从rdb文件.


#### void initServer(void)
server的初始化

    // server的所有的listening的fd，都放入eventloop里监听，对应的callback
    // 是acceptTcpHandler, unixsocket也类似，callback是acceptUnixHandler
    // 这两个handler都在networking.c里面
    for (j = 0; j < server.ipfd_count; j++) {
        if (aeCreateFileEvent(server.el, server.ipfd[j], AE_READABLE,
            acceptTcpHandler,NULL) == AE_ERR)
            {
                serverPanic(
                    "Unrecoverable error creating server.ipfd file event.");
            }
    }

#### int main(int argc, char **argv)
main函数.

    struct timeval tv;
    // 初始化(set proc title)
    #ifdef INIT_SETPROCTITLE_REPLACEMENT
        spt_init(argc, argv);
    #endif
    setlocale(LC_COLLATE,"");
    zmalloc_enable_thread_safeness();
    zmalloc_set_oom_handler(redisOutOfMemoryHandler);
    // 设置随机数函数的seed(根据time和pid生成)
    srand(time(NULL)^getpid());
    gettimeofday(&tv,NULL);
    // 设置hash的seed
    dictSetHashFunctionSeed(tv.tv_sec^tv.tv_usec^getpid());
    server.sentinel_mode = checkForSentinelMode(argc,argv);
    initServerConfig();
    moduleInitModulesSystem();
    // 保存启动的可执行文件和参数便于重启的时候需要用
    server.executable = getAbsolutePath(argv[0]);
    server.exec_argv = zmalloc(sizeof(char*)*(argc+1));
    server.exec_argv[argc] = NULL;
    for (j = 0; j < argc; j++) server.exec_argv[j] = zstrdup(argv[j]);

    if (server.sentinel_mode) {
        initSentinelConfig();
        initSentinel();
    }

    if (strstr(argv[0],"redis-check-rdb") != NULL)
        redis_check_rdb_main(argc,argv);

    if (argc >= 2) {
        j = 1; /* First option to parse in argv[] */
        sds options = sdsempty();
        char *configfile = NULL;

        // version和help参数解析
        if (strcmp(argv[1], "-v") == 0 ||
            strcmp(argv[1], "--version") == 0) version();
        if (strcmp(argv[1], "--help") == 0 ||
            strcmp(argv[1], "-h") == 0) usage();
        if (strcmp(argv[1], "--test-memory") == 0) {
            if (argc == 3) {
                memtest(atoi(argv[2]),50); exit(0);
            } else {
                fprintf(stderr,"Please specify the amount of memory to test in megabytes.\n");
                fprintf(stderr,"Example: ./redis-server --test-memory 4096\n\n");
                exit(1);
            }
        }

        if (argv[j][0] != '-' || argv[j][1] != '-') {
            configfile = argv[j];
            server.configfile = getAbsolutePath(configfile);
            zfree(server.exec_argv[j]);
            server.exec_argv[j] = zstrdup(server.configfile);
            j++;
        }

        // 参数解析(所有参数都是--XX)
        while(j != argc) {
            if (argv[j][0] == '-' && argv[j][1] == '-') {
                /* Option name */
                if (!strcmp(argv[j], "--check-rdb")) {
                    /* Argument has no options, need to skip for parsing. */
                    j++;
                    continue;
                }
                if (sdslen(options)) options = sdscat(options,"\n");
                options = sdscat(options,argv[j]+2);
                options = sdscat(options," ");
            } else {
                /* Option argument */
                options = sdscatrepr(options,argv[j],strlen(argv[j]));
                options = sdscat(options," ");
            }
            j++;
        }
        if (server.sentinel_mode && configfile && *configfile == '-') {
            serverLog(LL_WARNING,
                "Sentinel config from STDIN not allowed.");
            serverLog(LL_WARNING,
                "Sentinel needs config file on disk to save state.  Exiting...");
            exit(1);
        }
        resetServerSaveParams();
        loadServerConfig(configfile,options);
        sdsfree(options);
    } else {
        serverLog(LL_WARNING, "Warning: no config file specified, using the default config. In order to specify a config file use %s /path/to/%s.conf", argv[0], server.sentinel_mode ? "sentinel" : "redis");
    }

    server.supervised = redisIsSupervised(server.supervised_mode);
    int background = server.daemonize && !server.supervised;
    if (background) daemonize();

    initServer();
    if (background || server.pidfile) createPidFile();
    redisSetProcTitle(argv[0]);
    redisAsciiArt();
    checkTcpBacklogSettings();

    if (!server.sentinel_mode) {
        /* Things not needed when running in Sentinel mode. */
        serverLog(LL_WARNING,"Server started, Redis version " REDIS_VERSION);
    #ifdef __linux__
        linuxMemoryWarnings();
    #endif
        moduleLoadFromQueue();
        loadDataFromDisk();
        if (server.cluster_enabled) {
            if (verifyClusterConfigWithData() == C_ERR) {
                serverLog(LL_WARNING,
                    "You can't have keys in a DB different than DB 0 when in "
                    "Cluster mode. Exiting.");
                exit(1);
            }
        }
        if (server.ipfd_count > 0)
            serverLog(LL_NOTICE,"The server is now ready to accept connections on port %d", server.port);
        if (server.sofd > 0)
            serverLog(LL_NOTICE,"The server is now ready to accept connections at %s", server.unixsocket);
    } else {
        sentinelIsRunning();
    }

    if (server.maxmemory > 0 && server.maxmemory < 1024*1024) {
        serverLog(LL_WARNING,"WARNING: You specified a maxmemory value that is less than 1MB (current value is %llu bytes). Are you sure this is what you really want?", server.maxmemory);
    }
    aeSetBeforeSleepProc(server.el,beforeSleep);
    aeMain(server.el);
    aeDeleteEventLoop(server.el);
    return 0;

### Reference:
1. [github: redis server](https://github.com/antirez/redis/blob/unstable/src/server.c)
