+++
title = "代码阅读: redis-tlist"
date = "2018-03-03"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-tlist"
Authors = "Xudong"
+++

这里是对list相关命令的处理. 支撑list的数据结构是quicklist.

#### void listTypePush(robj *subject, robj *value, int where)

    if (subject->encoding == OBJ_ENCODING_QUICKLIST) {
        int pos = (where == LIST_HEAD) ? QUICKLIST_HEAD : QUICKLIST_TAIL;
        value = getDecodedObject(value);
        size_t len = sdslen(value->ptr);
        quicklistPush(subject->ptr, value->ptr, len, pos);
        decrRefCount(value);
    } else {
        serverPanic("Unknown list encoding");
    }

#### robj *listTypePop(robj *subject, int where)
从list中pop数据.

#### listTypeIterator *listTypeInitIterator(robj *subject, long index, unsigned char direction)
返回一个iterator, 从list的某个index开始.

#### int listTypeNext(listTypeIterator *li, listTypeEntry *entry)
迭代器获取下一个元素, entry用来接受返回的元素.

#### robj *listTypeGet(listTypeEntry *entry)
因为entry实际上是ziplist的一个元素，可以存一个string或者一个整数，这里是对字符串和整数统一处理下返回一个robj.

#### void listTypeInsert(listTypeEntry *entry, robj *value, int where)
根据where确定的方向向quicklist中插入一个元素.

    value = getDecodedObject(value);
    sds str = value->ptr;
    quicklistInsertAfter((quicklist *)entry->entry.quicklist,
                         &entry->entry, str, len);

#### int listTypeEqual(listTypeEntry *entry, robj *o)
比较当前位置的entry和robj是否相等.

#### void listTypeDelete(listTypeIterator *iter, listTypeEntry *entry)
删除元素

#### void listTypeConvert(robj *subject, int enc)
从ziplist创建一个quicklist.


#### void pushGenericCommand(client *c, int where)
这个是lpush, rpush的实现

    robj *lobj = lookupKeyWrite(c->db,c->argv[1]);
    // 如果key存在但是类型不是list就报错
    if (lobj && lobj->type != OBJ_LIST) {
        addReply(c,shared.wrongtypeerr);
        return;
    }
    // lpush, rpush可以一次push若干个，这里是一个个的push
    // 可以直接push一个不存在的key, 这样会先创建一个空的list
    for (j = 2; j < c->argc; j++) {
        if (!lobj) {
            lobj = createQuicklistObject();
            quicklistSetOptions(lobj->ptr, server.list_max_ziplist_size,
                                server.list_compress_depth);
            dbAdd(c->db,c->argv[1],lobj);
        }
        listTypePush(lobj,c->argv[j],where);
        pushed++;
    }
    addReplyLongLong(c, (lobj ? listTypeLength(lobj) : 0));
    if (pushed) {
        // 对key进行修改后的惯常的两个通知动作
        char *event = (where == LIST_HEAD) ? "lpush" : "rpush";
        signalModifiedKey(c->db,c->argv[1]);
        notifyKeyspaceEvent(NOTIFY_LIST,event,c->argv[1],c->db->id);
    }

#### void pushxGenericCommand(client *c, int where)
lpushx, rpushx的实现, 先查找key，不存在就报错，其它跟push实现类似.

#### void linsertCommand(client *c)
linsert的实现，创建一个iterator, 挨着迭代直到找到目标元素然后插入

    iter = listTypeInitIterator(subject,0,LIST_TAIL);
    while (listTypeNext(iter,&entry)) {
        if (listTypeEqual(&entry,c->argv[3])) {
            listTypeInsert(&entry,c->argv[4],where);
            inserted = 1;
            break;
        }
    }

#### void lindexCommand(client *c)
根据index获取元素, 支持负数序号

#### void lsetCommand(client *c)
替换元素

    int replaced = quicklistReplaceAtIndex(ql, index,
                                               value->ptr, sdslen(value->ptr));

#### void popGenericCommand(client *c, int where)
lpop, rpop的实现

    // pop一个元素出来，如果pop完了list为空，就删除这个key
    robj *value = listTypePop(o,where);
    if (value == NULL) {
        addReply(c,shared.nullbulk);
    } else {
        char *event = (where == LIST_HEAD) ? "lpop" : "rpop";
        addReplyBulk(c,value);
        decrRefCount(value);
        notifyKeyspaceEvent(NOTIFY_LIST,event,c->argv[1],c->db->id);
        if (listTypeLength(o) == 0) {
            notifyKeyspaceEvent(NOTIFY_GENERIC,"del",
                                c->argv[1],c->db->id);
            dbDelete(c->db,c->argv[1]);
        }
        signalModifiedKey(c->db,c->argv[1]);
    }

#### void lrangeCommand(client *c)
先对start, end处理下，然后开始迭代




### RstTypeIterator *iter = listTypeInitIterator(o, start, LIST_TAIL);

    while(rangelen--) {
        listTypeEntry entry;
        // 获取下一个
        listTypeNext(iter, &entry);
        quicklistEntry *qe = &entry.entry;
        // 对字符串和整数分别处理
        if (qe->value) {
            addReplyBulkCBuffer(c,qe->value,qe->sz);
        } else {
            addReplyBulkLongLong(c,qe->longval);
        }
    }
    listTypeReleaseIterator(iter);

#### void ltrimCommand(client *c)
ltrim key start stop, key的value是一个list，将这个list从start到stop之外的都删除掉(start, stop都需要保留), start, stop可以是负序.

先对start, stop进行处理，然后调用quicklistDelRange删除两边的, 最后就是key修改的通知.

    quicklistDelRange(o->ptr,0,ltrim);
    quicklistDelRange(o->ptr,-rtrim,rtrim);

#### void lremCommand(client *c)
获取iterator，然后一个个的按需要删除

    if (toremove < 0) {
        toremove = -toremove;
        li = listTypeInitIterator(subject,-1,LIST_HEAD);
    } else {
        li = listTypeInitIterator(subject,0,LIST_TAIL);
    }
    listTypeEntry entry;
    while (listTypeNext(li,&entry)) {
        if (listTypeEqual(&entry,obj)) {
            listTypeDelete(li, &entry);
            server.dirty++;
            removed++;
            if (toremove && removed == toremove) break;
        }
    }

#### void rpoplpushCommand(client *c)
这个就是把pop和push两个动作放在一起作为一个原子操作(原子性有redis提供), 如果客户端自己实现就没发保证原子性.

在相关的key的通知方面，也是烂招rpop, lpush来处理的，所以keyspace通知里没有rpoplpush这个.

#### void blockForKeys(client *c, robj **keys, int numkeys, mstime_t timeout, robj *target)
这里是list的block操作的处理.

下面是c->bpop的数据结构定义

    typedef struct blockingState {
        mstime_t timeout;    // blocking的到达时间点
        dict *keys;          // blocking等待的keys
        robj *target;        // BRPOPLPUSH 等待接受这个key的list
        int numreplicas;     // 等待ack的副本数量
        long long reploffset;   /* Replication offset to reach. */
    } blockingState;

blockForKeys的主要代码

    c->bpop.timeout = timeout;
    c->bpop.target = target;

    if (target != NULL) incrRefCount(target);

    for (j = 0; j < numkeys; j++) {
        // 对于每个key, 将该key加入到c->bpop.keys这个set里(value为null)
        // 同时还需要加入到db->blocking_keys这个dict里面去, value是client组成的list, 如果list不存在就创建一个
        // 只所以用list是因为当多个client等待一个key的时候，需要按照等待的顺序pop
        if (dictAdd(c->bpop.keys,keys[j],NULL) != DICT_OK) continue;
        incrRefCount(keys[j]);
        de = dictFind(c->db->blocking_keys,keys[j]);
        if (de == NULL) {
            int retval;
            l = listCreate();
            retval = dictAdd(c->db->blocking_keys,keys[j],l);
            incrRefCount(keys[j]);
            serverAssertWithInfo(c,keys[j],retval == DICT_OK);
        } else {
            l = dictGetVal(de);
        }
        listAddNodeTail(l,c);
    }
    blockClient(c,BLOCKED_LIST);

blockClient这个函数(blocked.c)就是设置client为blocked，标记类型.

#### void unblockClientWaitingData(client *c)
这个跟上面的操作正好相反，将一个client的所有blocking的keys都清理掉.

遍历c->bpop.keys这个set, 从db->blocking_keys里面找该key对应的list, 从list中删除该client, 如果list为空，再把list也删掉.


#### void signalListAsReady(redisDb *db, robj *key)

    // 如果blocking_keys里没有，就到不了ready这步
    // 如果db->ready_keys里面已经有了，就不需要重复添加了
    if (dictFind(db->blocking_keys,key) == NULL) return;
    if (dictFind(db->ready_keys,key) != NULL) return;

    // 添加到server.ready_keys这个list里面去, 这里key可能重复，因为可能在不同的db
    rl = zmalloc(sizeof(*rl));
    rl->key = key;
    rl->db = db;
    incrRefCount(key);
    listAddNodeTail(server.ready_keys,rl);

    // 添加到db->ready_keys这个set里面去
    incrRefCount(key);
    serverAssert(dictAdd(db->ready_keys,key,NULL) == DICT_OK);

#### int serveClientBlockedOnList(client *receiver, robj *key, robj *dstkey, redisDb *db, robj *value, int where)
这里是pop后的处理，对于常规的pop，就是返回信息给客户端以及propagate处理.
对于BRPOPLPUSH, 这里调用rpoplpushHandlePush来进行lpush的动作, 然后进行RPOP, LPUSH这两个动作的propagate处理.

#### void handleClientsBlockedOnLists(void)
这个函数在每个命令, MULTI/EXEC块，以及lua脚本执行后都会执行，目的是将server.ready_keys里的key都清空.

    // 所有的ready_keys都需要被处理完
    while(listLength(server.ready_keys) != 0) {
        list *l;

        // l指向原来的server.ready_keys, server.ready_keys指向一个空的list
        l = server.ready_keys;
        server.ready_keys = listCreate();

        // 这个while会将l处理完毕，处理过程中可能会产生新的ready_keys, 这些会在外层的while循环里处理
        while(listLength(l) != 0) {
            listNode *ln = listFirst(l);
            readyList *rl = ln->value;
            dictDelete(rl->db->ready_keys,rl->key);

            robj *o = lookupKeyWrite(rl->db,rl->key);
            if (o != NULL && o->type == OBJ_LIST) {
                dictEntry *de;
                de = dictFind(rl->db->blocking_keys,rl->key);
                if (de) {
                    list *clients = dictGetVal(de);
                    int numclients = listLength(clients);

                    while(numclients--) {
                        // 取clients组成的list的第一个元素, 所以这里的的是先来先得模式
                        listNode *clientnode = listFirst(clients);
                        client *receiver = clientnode->value;
                        robj *dstkey = receiver->bpop.target;
                        int where = (receiver->lastcmd &&
                                     receiver->lastcmd->proc == blpopCommand) ?
                                    LIST_HEAD : LIST_TAIL;
                        // 从block的key对应的list里pop一个值出来消费
                        robj *value = listTypePop(o,where);
                        if (value) {
                            if (dstkey) incrRefCount(dstkey);
                            unblockClient(receiver);

                            // 如果消费失败(比如client连接断了), 将pop出来的元素push回去
                            if (serveClientBlockedOnList(receiver,
                                rl->key,dstkey,rl->db,value,
                                where) == C_ERR)
                            {
                                    listTypePush(o,value,where);
                            }

                            if (dstkey) decrRefCount(dstkey);
                            decrRefCount(value);
                        } else {
                            break;
                        }
                    }
                }

                if (listTypeLength(o) == 0) {
                    dbDelete(rl->db,rl->key);
                }
            }

            decrRefCount(rl->key);
            zfree(rl);
            listDelNode(l,ln);
        }
        listRelease(l); /* We have the new list on place at this point. */
    }
}

#### void blockingPopGenericCommand(client *c, int where)
遍历参数里面的所以key, 如果任何一个key对应的list可以pop value，就直接返回给客户端，然后结束了，这个跟non-blocking的处理是一样的.

如果所有的key对应的list都没能pop出一个value，就调用blockForKeys处理.


reference:
1. [github: redis tlist](https://github.com/antirez/redis/blob/unstable/src/t_list.c)
