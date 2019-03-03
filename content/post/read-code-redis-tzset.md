+++
title = "代码阅读: redis-tzset"
date = "2019-02-24"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-tzset"
Authors = "Xudong"
+++

zset是有序的set，底层实现是hash table加skip list或者ziplist, hash table负责实现
set，skiplist/ziplist维护顺序.

如果robj->encoding是OBJ_ENCODING_ZIPLIST则是ziplist实现，如果是OBJ_ENCODING_SKIPLIST则是skiplist实现.

#### void zsetConvertToZiplistIfNeeded(robj *zobj, size_t maxelelen)
OBJ_ZSET_MAX_ZIPLIST_ENTRIES默认是128, OBJ_ZSET_MAX_ZIPLIST_VALUE默认是64.

    if (zset->zsl->length <= server.zset_max_ziplist_entries &&
        maxelelen <= server.zset_max_ziplist_value)
            zsetConvert(zobj,OBJ_ENCODING_ZIPLIST);


### Reference:
1. [github: redis tzset](https://github.com/antirez/redis/blob/unstable/src/t_zset.c)
