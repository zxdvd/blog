+++
title = "代码阅读: redis-thash"
date = "2018-01-14"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-thash"
Authors = "Xudong"
+++

t_hash.c是对hget/hset/hmget/hmset/hdel/hkeys/hvals等命令的处理，以及背后的增删改查逻辑.

看过object.c都知道所有key的value都是以robj结构存的，对于hash，robj有两种表达方式.
1. 对于比较小的hash键值对，以ziplist的形式存在(实质是一个长字符串)，比如第0个是key，第1个是value，第2个是key，第3个是value，这样紧挨着，对应的robj->encoding为`OBJ_ENCODING_ZIPLIST`.
2. 对于ziplist超过一定长度时，需要转换成真正的hash表结构(dict.c), 这时它的robj->encoding为`OBJ_ENCODING_HT`.

在每次插入/更新的时候都会检查key/value的长度，超过了`server.hash_max_ziplist_value`(默认64字节), 就会将ziplist的形式转换成hash table的形式.
插入之后还会检查总的键值对数量，超过`server.hash_max_ziplist_entries`(默认是512个)时，也会转换成hash table.

#### void hashTypeTryConversion(robj *o, robj **argv, int start, int end)
这个是检查传过来的多个key，value，如果字符串长度超过`server.hash_max_ziplist_value`, 就将这个ziplist的hash结构转成hash表.

#### int hashTypeGetFromZiplist(robj *o, sds field, unsigned char **vstr, unsigned int *vlen, long long *vll)
从o这个ziplist类型的robj里面根据key来获取value的指针位置, 以及字符串长度，如果能转换成long long类型，也返回该值.

#### sds hashTypeGetFromHashTable(robj *o, sds field)
从hash表结构里根据key来获取对应的value.

#### hashTypeGetValue
自动根据robj的类型来确定调上面两个函数中的某一个，然后对应的value.

#### int hashTypeSet(robj *o, sds field, sds value, int flags)
在hash表里添加/更新一对新的key/value.
1. 对于ziplist形式，如果key存在，定位到后面的value，先删除再插入.
2. 如果key不存在，在ziplist尾部添加这一对key/value.
3. 如果是hash表结构，相对简单，更新或者插入即可.

#### unsigned long hashTypeLength(const robj *o)
获取hash的长度，如果是ziplist形式，整个list元素个数除以2就是了，如果是dict，通过dictSize获取总长度.

### Reference:
1. [github: redis zmalloc](https://github.com/antirez/redis/blob/unstable/src/t_hash.c)
