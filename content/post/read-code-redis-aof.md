+++
title = "代码阅读: redis-aof"
date = "2018-03-17"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-aof"
Authors = "Xudong"
+++

aof的处理.

redisServer结构里跟aof相关的

    int aof_state;                  /* AOF_(ON|OFF|WAIT_REWRITE) */
    int aof_fsync;                  /* Kind of fsync() policy */
    char *aof_filename;             /* Name of the AOF file */
    int aof_no_fsync_on_rewrite;    /* Don't fsync if a rewrite is in prog. */
    int aof_rewrite_perc;           /* Rewrite AOF if % growth is > M and... */
    off_t aof_rewrite_min_size;     /* the AOF file is at least N bytes. */
    off_t aof_rewrite_base_size;    /* AOF size on latest startup or rewrite. */
    off_t aof_current_size;         // 调用fstat获取aof_fd的文件大小
    int aof_rewrite_scheduled;      /* Rewrite once BGSAVE terminates. */
    pid_t aof_child_pid;            /* PID if rewriting process */
    list *aof_rewrite_buf_blocks;   /* Hold changes during an AOF rewrite. */
    sds aof_buf;      /* AOF buffer, written before entering the event loop */
    int aof_fd;       /* File descriptor of currently selected AOF file */
    int aof_selected_db; /* Currently selected DB in AOF */
    time_t aof_flush_postponed_start; /* UNIX time of postponed AOF flush */
    time_t aof_last_fsync;            /* UNIX time of last fsync() */
    time_t aof_rewrite_time_last;   /* Time used by last AOF rewrite run. */
    time_t aof_rewrite_time_start;  /* Current AOF rewrite start time. */
    int aof_lastbgrewrite_status;   /* C_OK or C_ERR */
    unsigned long aof_delayed_fsync;  /* delayed AOF fsync() counter */
    int aof_rewrite_incremental_fsync;/* fsync incrementally while rewriting? */
    int aof_last_write_status;      /* C_OK or C_ERR */
    int aof_last_write_errno;       /* Valid if aof_last_write_status is ERR */
    int aof_load_truncated;         /* Don't stop on unexpected AOF EOF. */
    // 进程间通信用的3对pipe
    int aof_pipe_write_data_to_child;
    int aof_pipe_read_data_from_parent;
    int aof_pipe_write_ack_to_parent;
    int aof_pipe_read_ack_from_child;
    int aof_pipe_write_ack_to_child;
    int aof_pipe_read_ack_from_parent;
    int aof_stop_sending_diff;     /* If true stop sending accumulated diffs
                                      to child process. */
    sds aof_child_diff;             /* AOF diff accumulator child side. */



#### void aofChildWriteDiffData(aeEventLoop *el, int fd, void *privdata, int mask)
将aof的buffer list写入文件.
这里是一个`while(1)`循环将整个buffer list全部写入.

    if (block->used > 0) {
        nwritten = write(server.aof_pipe_write_data_to_child,
                         block->buf,block->used);
        if (nwritten <= 0) return;
        // 通过memmove将已经写入的部分给覆盖掉
        memmove(block->buf,block->buf+nwritten,block->used-nwritten);
        block->used -= nwritten;
    }

#### void aofRewriteBufferAppend(unsigned char *s, unsigned long len)
这个是写入数据到buffer.

    // 找到list里最后一个buffer开始append数据
    listNode *ln = listLast(server.aof_rewrite_buf_blocks);
    aofrwblock *block = ln ? ln->value : NULL;
    while(len) {
        if (block) {
            unsigned long thislen = (block->free < len) ? block->free : len;
            if (thislen) {
                memcpy(block->buf+block->used, s, thislen);
                block->used += thislen;
                block->free -= thislen;
                s += thislen;
                len -= thislen;
            }
        }
        // 最后一个block都写满了还有数据要写的话就需要增加buffer了
        if (len) {
            block = zmalloc(sizeof(*block));
            block->free = AOF_RW_BUF_BLOCK_SIZE;
            block->used = 0;
            listAddNodeTail(server.aof_rewrite_buf_blocks,block);
        }
    }

#### ssize_t aofRewriteBufferWrite(int fd)
将buffer list全部写入到文件, 但是对buffer本身不做移动或者释放的动作.

#### void aof_background_fsync(int fd)
写入到文件描述符里其实还在内存里，如果想落盘，需要fsync或者fdatasync. 但是为了不阻塞主线程，将这个放到background进程里去处理.

#### void stopAppendOnly(void)
先将当前的数据落盘然后关闭描述符，然后kill, wait子进程. 释放buffer list.

#### sds catAppendOnlyGenericCommand(sds dst, int argc, robj **argv)
这个是将命令的参数长度以及具体参数打包成一个长串便于写入文件.
格式是这样的

        `*${1+len(argc)}\r\n$${1+len(argv)}\r\n${argv}\r\n`

比如`lpush key value1`会记录成`*3\r\n$5\r\nlpush\r\n$3\r\nkey\r\n$6\r\nvalue1\r\n`

#### sds catAppendOnlyExpireAtCommand(sds buf, struct redisCommand *cmd, robj *key, robj *seconds)
因为expire相关的几个命令单位有秒和毫秒，时间有相当跟绝对，这里统一转换成绝对时间的，也就是PEXPIREAT命令.

#### void feedAppendOnlyFile(struct redisCommand *cmd, int dictid, robj **argv, int argc)
将各种命令进行aof格式化转换.
首先如果dictid跟server.aof_selected_db不一样的话，会拼一个`select dictid`的命令，append到buf里去.

然后把命令分成几类，对于epxire的那几个命令用上面的`catAppendOnlyExpireAtCommand`函数转换处理; 对于setex, psetex这两个拆分成set跟expire两条命令处理; 剩余其他的那些命令就是常规处理(catAppendOnlyGenericCommand).

最后如果aof_state是on的话，整个buf append到server.aof_buf后面.
如果background_rewrite进程在的话，会append到server.aof_rewrite_buf_blocks这个list尾部.

#### int loadAppendOnlyFile(char *filename)
载入一个aof的log文件.
首先设置aof_state为off, 然后调用rdb.c里的startLoading函数来初始化一下, 然后是一个循环解析log文件的过程

    // 准备一个假的client, 后面log里读取的命令用它来执行
    fakeClient = createFakeClient();
    while(1) {
        // 读取一行或者buf[128]个字符到buf(看来log文件有每行128字符的限制)
        if (fgets(buf,sizeof(buf),fp) == NULL) {
            // 如果是到行尾了就正常退出循环, 否则就是出错了
            if (feof(fp))
                break;
            else
                goto readerr;
        }
        // 第一个字符是*, 并且不能只有一个*
        if (buf[0] != '*') goto fmterr;
        if (buf[1] == '\0') goto readerr;
        // *后面紧跟着的是表示命令共用多少个参数, argc
        argc = atoi(buf+1);
        if (argc < 1) goto fmterr;
        // 因为有argc个参数，这里就对应的先申请内存
        argv = zmalloc(sizeof(robj*)*argc);
        fakeClient->argc = argc;
        fakeClient->argv = argv;
        // 后面的argc个参数一个个的循环读取
        for (j = 0; j < argc; j++) {
            if (fgets(buf,sizeof(buf),fp) == NULL) {
                    // DEAL_WITH_ERROR
            }
            // 参数的第一个字符一定是$
            if (buf[0] != '$') goto fmterr;
            // 第一个字符后面开始的是表示参数长度的字符型整数
            len = strtol(buf+1,NULL,10);
            // 创建一个该长度的sds字符串
            argsds = sdsnewlen(NULL,len);
            // 继续读取len个字符到argsds这个串
            if (len && fread(argsds,len,1,fp) == 0) {
                // DEAL_WITH_ERROR
            }
            // 生成robj
            argv[j] = createObject(OBJ_STRING,argsds);
            // 读取后面\r\n这两个字符, 不做处理也就是skip掉
            if (fread(buf,2,1,fp) == 0) {
                // DEAL_WITH_ERROR
            }
        }

        // 到这里一个完整的命令读取出来了，该执行它了
        // 从commandTable里通过名字找命令对应的函数指针
        cmd = lookupCommand(argv[0]->ptr);
        if (!cmd) {
            serverLog(LL_WARNING,"Unknown command '%s' reading the append only file", (char*)argv[0]->ptr);
            exit(1);
        }
        // 假的client去执行命令
        fakeClient->cmd = cmd;
        cmd->proc(fakeClient);

        serverAssert(fakeClient->bufpos == 0 && listLength(fakeClient->reply) == 0);
        serverAssert((fakeClient->flags & CLIENT_BLOCKED) == 0);
        freeFakeClientArgv(fakeClient);
        fakeClient->cmd = NULL;
        if (server.aof_load_truncated) valid_up_to = ftello(fp);
    }

在进行AOF rewrite部分前，先介绍下rio.c里的rioWriteBulkXXX函数, 这些是用来帮助生成log字符串的.

#### size_t rioWriteBulkCount(rio *r, char prefix, int count)
帮助生成`<prefix><count>\r\n`格式的串

    cbuf[0] = prefix;
    clen = 1+ll2string(cbuf+1,sizeof(cbuf)-1,count);
    cbuf[clen++] = '\r';
    cbuf[clen++] = '\n';
    if (rioWrite(r,cbuf,clen) == 0) return 0;
}

#### size_t rioWriteBulkString(rio *r, const char *buf, size_t len)
帮助生成`$<count>\r\n<payload_string>\r\n`格式的串

    if ((nwritten = rioWriteBulkCount(r,'$',len)) == 0) return 0;
    if (len > 0 && rioWrite(r,buf,len) == 0) return 0;
    if (rioWrite(r,"\r\n",2) == 0) return 0;
    return nwritten+len+2;

#### rioWriteBulkLongLong, rioWriteBulkDouble
这两个都是将数字转换成string然后调用rioWriteBulkString写入的.

rio.c里的帮助函数就这些了.

#### int rioWriteBulkObject(rio *r, robj *obj)
根据类型调用rio.c里的helper来生成log格式

    if (obj->encoding == OBJ_ENCODING_INT) {
        return rioWriteBulkLongLong(r,(long)obj->ptr);
    } else if (sdsEncodedObject(obj)) {
        return rioWriteBulkString(r,obj->ptr,sdslen(obj->ptr));
    }

#### int rewriteListObject(rio *r, robj *key, robj *o)
这里是把list变成一个RPUSH key v1 v2 v3 v4....的log结构，而且是每64个key进行切割, 也就是一个包含128个value的list会拆成2个RPUSH命令.

    while (quicklistNext(li,&entry)) {
        if (count == 0) {
            // 每一组开始前先插入命令参数个数以及RPUSH KEY
            int cmd_items = (items > AOF_REWRITE_ITEMS_PER_CMD) ?
                AOF_REWRITE_ITEMS_PER_CMD : items;
            // 格式加2是因为参数要包含`RPUSH KEY`这两个
            if (rioWriteBulkCount(r,'*',2+cmd_items) == 0) return 0;
            if (rioWriteBulkString(r,"RPUSH",5) == 0) return 0;
            if (rioWriteBulkObject(r,key) == 0) return 0;
        }
        // 写入value
        if (entry.value) {
            if (rioWriteBulkString(r,(char*)entry.value,entry.sz) == 0) return 0;
        } else {
            if (rioWriteBulkLongLong(r,entry.longval) == 0) return 0;
        }
        // 每64个就重新开始
        if (++count == AOF_REWRITE_ITEMS_PER_CMD) count = 0;
        items--;
    }

#### int rewriteSetObject(rio *r, robj *key, robj *o)
set的处理，跟list基本类似，就是变成SADD  KEY s1 s2 s3..., 64个一分组，这里要根据set是intset还是dict实现分别处理.

#### int rewriteSortedSetObject(rio *r, robj *key, robj *o)
zset的处理，跟前面类似，变成`ZADD KEY score1 mem1 score2 mem2...`, 64个一组, 根据zset是ZIPLIST还是SKIPLIST分别处理.

#### int rewriteHashObject(rio *r, robj *key, robj *o)
hash的处理，变成`HMSET KEY k1 v1 k2 v2...`的命令，64个一组.

#### ssize_t aofReadDiffFromParent(void)
这个是子进程执行的，从pipe里读取数据，pipe另一端是父进程

    char buf[65536]; /* Default pipe buffer size on most Linux systems. */
    while ((nread =
            read(server.aof_pipe_read_data_from_parent,buf,sizeof(buf))) > 0) {
        server.aof_child_diff = sdscatlen(server.aof_child_diff,buf,nread);
        total += nread;
    }

#### int rewriteAppendOnlyFile(char *filename)
这个是将redis的所有db里的数据写入文件的处理(这个是子进程里运行的)

    // 一个个db的处理
    for (j = 0; j < server.dbnum; j++) {
        // 先拼一个select dbid的命令(这里缺少id)
        char selectcmd[] = "*2\r\n$6\r\nSELECT\r\n";
        redisDb *db = server.db+j;
        dict *d = db->dict;
        if (dictSize(d) == 0) continue;
        di = dictGetSafeIterator(d);
        if (!di) {
            fclose(fp);
            return C_ERR;
        }

        // 将select dbid这个命令写入
        if (rioWrite(&aof,selectcmd,sizeof(selectcmd)-1) == 0) goto werr;
        if (rioWriteBulkLongLong(&aof,j) == 0) goto werr;
        // 开始遍历整个dict，一个个的写入
        while((de = dictNext(di)) != NULL) {
            sds keystr;
            robj key, *o;
            long long expiretime;

            keystr = dictGetKey(de);
            o = dictGetVal(de);
            initStaticStringObject(key,keystr);

            expiretime = getExpire(db,&key);
            if (expiretime != -1 && expiretime < now) continue;
            // 按照string, list, set, zset, hash, module这几个类别分别处理
            if (o->type == OBJ_STRING) {
                // 拼一个set key value的命令
                char cmd[]="*3\r\n$3\r\nSET\r\n";
                if (rioWrite(&aof,cmd,sizeof(cmd)-1) == 0) goto werr;
                if (rioWriteBulkObject(&aof,&key) == 0) goto werr;
                if (rioWriteBulkObject(&aof,o) == 0) goto werr;
            } else if (o->type == OBJ_LIST) {
                if (rewriteListObject(&aof,&key,o) == 0) goto werr;
            } else if (o->type == OBJ_SET) {
                if (rewriteSetObject(&aof,&key,o) == 0) goto werr;
            } else if (o->type == OBJ_ZSET) {
                if (rewriteSortedSetObject(&aof,&key,o) == 0) goto werr;
            } else if (o->type == OBJ_HASH) {
                if (rewriteHashObject(&aof,&key,o) == 0) goto werr;
            } else if (o->type == OBJ_MODULE) {
                if (rewriteModuleObject(&aof,&key,o) == 0) goto werr;
            } else {
                serverPanic("Unknown object type");
            }
            // 对于有过期时间设置的，拼一个PEXPIREAT的命令
            if (expiretime != -1) {
                char cmd[]="*3\r\n$9\r\nPEXPIREAT\r\n";
                if (rioWrite(&aof,cmd,sizeof(cmd)-1) == 0) goto werr;
                if (rioWriteBulkObject(&aof,&key) == 0) goto werr;
                if (rioWriteBulkLongLong(&aof,expiretime) == 0) goto werr;
            }
            // 每处理10k大小就从父进程读一些放到server.aof_child_diff
            if (aof.processed_bytes > processed+1024*10) {
                processed = aof.processed_bytes;
                aofReadDiffFromParent();
            }
        }
        dictReleaseIterator(di);
        di = NULL;
    }

#### void aofChildPipeReadable(aeEventLoop *el, int fd, void *privdata, int mask)
接受child的ack('!'字符)，并回复确认

    if (read(fd,&byte,1) == 1 && byte == '!') {
        server.aof_stop_sending_diff = 1;
        if (write(server.aof_pipe_write_ack_to_child,"!",1) != 1) {
                // DEAL_WITH_ERROR
        }
    }
    aeDeleteFileEvent(server.el,server.aof_pipe_read_ack_from_child,AE_READABLE);

#### int aofCreatePipes(void)
创建了3对pipe用于进程间通信

    int fds[6] = {-1, -1, -1, -1, -1, -1};
    if (pipe(fds) == -1) goto error; /* parent -> children data. */
    if (pipe(fds+2) == -1) goto error; /* children -> parent ack. */
    if (pipe(fds+4) == -1) goto error; /* children -> parent ack. */
    /* Parent -> children data is non blocking. */
    if (anetNonBlock(NULL,fds[0]) != ANET_OK) goto error;
    if (anetNonBlock(NULL,fds[1]) != ANET_OK) goto error;
    if (aeCreateFileEvent(server.el, fds[2], AE_READABLE, aofChildPipeReadable, NULL) == AE_ERR) goto error;

    server.aof_pipe_write_data_to_child = fds[1];
    server.aof_pipe_read_data_from_parent = fds[0];
    server.aof_pipe_write_ack_to_parent = fds[3];
    server.aof_pipe_read_ack_from_child = fds[2];
    server.aof_pipe_write_ack_to_child = fds[5];
    server.aof_pipe_read_ack_from_parent = fds[4];
    server.aof_stop_sending_diff = 0;


#### int rewriteAppendOnlyFileBackground(void)
aof的处理过程, 这里首先fork出一个进程，不影响主进程，这个进程专门处理aof.

    // 如果aof或者rdb进程已存在就返回错误
    if (server.aof_child_pid != -1 || server.rdb_child_pid != -1) return C_ERR;
    // 创建进程间通信的pipe
    if (aofCreatePipes() != C_OK) return C_ERR;
    start = ustime();
    if ((childpid = fork()) == 0) {
        char tmpfile[256];
        // 子进程里关闭socket
        closeListeningSockets(0);
        redisSetProcTitle("redis-aof-rewrite");
        snprintf(tmpfile,256,"temp-rewriteaof-bg-%d.aof", (int) getpid());
        if (rewriteAppendOnlyFile(tmpfile) == C_OK) {
            size_t private_dirty = zmalloc_get_private_dirty();

            if (private_dirty) {
                serverLog(LL_NOTICE,
                    "AOF rewrite: %zu MB of memory used by copy-on-write",
                    private_dirty/(1024*1024));
            }
            exitFromChild(0);
        } else {
            exitFromChild(1);
        }
    } else {            // 父进程
        server.stat_fork_time = ustime()-start;
        server.stat_fork_rate = (double) zmalloc_used_memory() * 1000000 / server.stat_fork_time / (1024*1024*1024); /* GB per second. */
        latencyAddSampleIfNeeded("fork",server.stat_fork_time/1000);
        if (childpid == -1) {
                DEAL_WITH_FORK_ERROR
        }
        server.aof_rewrite_scheduled = 0;
        server.aof_rewrite_time_start = time(NULL);
        server.aof_child_pid = childpid;
        updateDictResizePolicy();
        server.aof_selected_db = -1;
        replicationScriptCacheFlush();
        return C_OK;
    }
    // 将userspace的buffer提交给kernel，并sync落盘
    if (fflush(fp) == EOF) goto werr;
    if (fsync(fileno(fp)) == -1) goto werr;

#### void backgroundRewriteDoneHandler(int exitcode, int bysignal)
这里是上面fork处理的进程结束后调用，是server.c里面有一段wait的代码(省略了一些其他的)

    if ((pid = wait3(&statloc,WNOHANG,NULL)) != 0) {
        if (pid == server.aof_child_pid) {
            backgroundRewriteDoneHandler(exitcode,bysignal);
        }
    }

后面是rename的处理.

### Reference:
1. [github: redis aof](https://github.com/antirez/redis/blob/unstable/src/aof.c)
