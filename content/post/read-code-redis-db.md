+++
title = "代码阅读: redis-db"
date = "2018-03-10"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-db"
Authors = "Xudong"
+++

#### robj *lookupKey(redisDb *db, robj *key, int flags)
查找key并返回value, 更新lru信息.

    dictEntry *de = dictFind(db->dict,key->ptr);
    if (de) {
        robj *val = dictGetVal(de);
        // 如果rdb, aof在工作, 或者flags指定NOTOUCH不不更新lru
        // 需要根据memory策略是LRU还是LFU分别处理
        if (server.rdb_child_pid == -1 &&
            server.aof_child_pid == -1 &&
            !(flags & LOOKUP_NOTOUCH))
        {
            if (server.maxmemory_policy & MAXMEMORY_FLAG_LFU) {
                unsigned long ldt = val->lru >> 8;
                unsigned long counter = LFULogIncr(val->lru & 255);
                val->lru = (ldt << 8) | counter;
            } else {
                val->lru = LRU_CLOCK();
            }
        }
        return val;
    }


#### robj *lookupKeyReadWithFlags(redisDb *db, robj *key, int flags)
这个主要是先检查key是否expired, 如果expired会删除对应的key(在expireIfNeeded函数里), 对于master节点直接返回null, client节点会有一些额外的判断.

然后会更新下server的hit和miss统计信息.

#### void dbAdd(redisDb *db, robj *key, robj *val)
dict添加key, value的处理, 如果是list的话会调用signalListAsReady, 这是有blocking的list操作比如rpop需要的.

    if (val->type == OBJ_LIST) signalListAsReady(db, key);

#### void dbOverwrite(redisDb *db, robj *key, robj *val)
调用dictReplace, 然后内存为LFU策略的时候，需要从old_value里将lru拷出来存到新的里面, 不然lru会按照一个全新的key来算. 对于LRU来说就不需要，因为val在生成的时候LRU就更新成当前时间了.

#### void setKey(redisDb *db, robj *key, robj *val)
这是一个upsert操作.

#### robj *dbRandomKey(redisDb *db)
调用dictGetRandomKey获取一个随机key, 如果dict是空的返回NULL, 对于获取的key还会检查是否expire, 如果expire了会再random一个.

#### int dbSyncDelete(redisDb *db, robj *key)
从db->expires, db->dict里删除key.

#### int dbDelete(redisDb *db, robj *key)
优先使用异步删除, 不支持的话就用同步的方式删除.

    return server.lazyfree_lazy_server_del ? dbAsyncDelete(db,key) :
                                             dbSyncDelete(db,key);

#### long long emptyDb(int dbnum, int flags, void(callback)(void*))
清空一个或者所有的db, 也有async的方式, 对于async方式就是创建两个空的dict替代原来的db上的expires和dict, 然后将旧的两个dict放入后天进程中处理，这个在bio模块里.

#### void typeCommand(client *c)
返回value的类型，比如none, string, list, set, zset, hash.

#### void propagateExpire(redisDb *db, robj *key, int lazy)
将key过期的信息同步给aof和slaves.

#### int expireIfNeeded(redisDb *db, robj *key)
检测和处理key过期

    mstime_t when = getExpire(db,key);
    mstime_t now;
    // when = -1表示这是一个持久化的key
    if (when < 0) return 0;
    if (server.loading) return 0;

    // 对于正在运行的lua脚本, 认为当前时间是lua开始的时间(为了一致性)
    now = server.lua_caller ? server.lua_time_start : mstime();

    // 对于slaves, 只返回结果，对于过期的key不做实际的处理
    if (server.masterhost != NULL) return now > when;

    if (now <= when) return 0;

    // master节点处理key过期
    server.stat_expiredkeys++;
    propagateExpire(db,key,server.lazyfree_lazy_expire);
    notifyKeyspaceEvent(NOTIFY_EXPIRED,
        "expired",key,db->id);
    return server.lazyfree_lazy_expire ? dbAsyncDelete(db,key) :
                                         dbSyncDelete(db,key);




### Reference:
1. [github: redis db](https://github.com/antirez/redis/blob/unstable/src/db.c)
