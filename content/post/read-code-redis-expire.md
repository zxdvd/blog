+++
title = "代码阅读: redis-expire"
date = "2018-02-18"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-expire"
Authors = "Xudong"
+++

关于expire keys的处理，也就是主要是db->expires.

#### int activeExpireCycleTryExpire(redisDb *db, dictEntry *de, long long now)
检查一个key是否过期，如果过期了就删除，有一些hook动作，比如统计，keyspaceEvent之类的

    // 获取过期时间
    long long t = dictGetSignedIntegerVal(de);
    if (now > t) {
        sds key = dictGetKey(de);
        robj *keyobj = createStringObject(key,sdslen(key));

        propagateExpire(db,keyobj,server.lazyfree_lazy_expire);
        if (server.lazyfree_lazy_expire)
            dbAsyncDelete(db,keyobj);
        else
            dbSyncDelete(db,keyobj);
        notifyKeyspaceEvent(NOTIFY_EXPIRED,
            "expired",keyobj,db->id);
        decrRefCount(keyobj);
        server.stat_expiredkeys++;
        return 1;
    } else {
        return 0;
    }

#### void activeExpireCycle(int type)
这个是redis的周期性循环动作之一，清理各个db的expires里的过期的key，
里面有不少static的变量是考虑到redis基本是单线程处理的，不能阻塞太长时间，
所有基本是expire一些然后记住当前db，处理时间等，下次接着做.

这个清理还分FAST和SLOW两种，FAST执行1ms左右，并且间隔要超过2ms, SLOW默认执行redis一个周期的25%，按照默认10HZ的设置也就是25ms.

执行的过程就是所有的db一个个循环随机取key，调用上面的 activeExpireCycleTryExpire，直到时间到了，如果key太少导致slots填充率不到1%也会提前结束.

如果expired keys超过5个也会提前结束.

#### void expireGenericCommand(client *c, long long basetime, int unit)
这个是EXPIRE, PEXPIRE, EXPIREAT, PEXPIREAT的实际实现.

    // 第一行处理单位，第二行处理相当时间和绝对时间的转换
    if (unit == UNIT_SECONDS) when *= 1000;
    // 对于EXPIRE这样的basetime就是now, 对于EXPIREAT basetime是0
    when += basetime;

    // 如果需要设置的时间是过去的时间，并且不是在AOF loading阶段, 也不是slave节点(masterhost是slave才有的属性), 就执行DEL操作
    // 否则就正常的expire处理，这两个过程都有关联的signalModifiedKey操作和keyspaceEvent通知
    if (when <= mstime() && !server.loading && !server.masterhost) {
      DO_DEL_KEY;
    } else {
      DO_SET_EXPIRE_KEY;
    }


#### void ttlGenericCommand(client *c, int output_ms)
TTL命令的实现，返回一个key的剩余时间. -2是key不存在, -1是key没有设置expire.

    // 这里是TTL返回秒数的时候做了四舍五入
    addReplyLongLong(c,output_ms ? ttl : ((ttl+500)/1000));

#### void persistCommand(client *c)
将一个key的expire设置删除掉(该key变成持久保存的), key不存在或者在expire里不存在都返回0，成功返回1.


### Reference:
1. [github: redis expire](https://github.com/antirez/redis/blob/unstable/src/expire.c)
