+++
title = "代码阅读: redis-zipmap"
date = "2017-12-30"
categories = ["Linux"]
tags = ["read-code", "c", "redis", "zipmap"]
slug = "read-code-redis-zipmap"
Authors = "Xudong"
+++

zipmap这个数据结构开始是为了hash存在的，节省内存，达到一定条件和数量后变成了hash。后来zipmap被ziplist取代了(commit ebd85e, version 2.9.2)，代码被保留了但是没有被用到.

zipmap是为了节省内存空间，将键值对拼接成string来存储. 下面是string的布局

     * Memory layout of a zipmap, for the map "foo" => "bar", "hello" => "world":
     *
     * <zmlen><len>"foo"<len><free>"bar"<len>"hello"<len><free>"world"
最前面的<zmlen>一个字节是表示有多少个键值对(不是整个zipmap的长度), 如果>=254个这个byte的值就记为254, 这个时候如果要获取长度需要遍历整个字符串.
<len>是紧跟着的key的长度, 这是个变长字段, 如果第一个字节小于254，这个字节就代表后面的key的长度，这个时候len的长度就是1字节; 如果第一个字节等于254，这个时候len的总长度是5个字节，后面四个字节(unsigned int)是key的长度.
这里额外说一点，对于四个字节的`unsigned int`存在char *里，那么读出来的时候自然就得考虑大端和小端问题了，redis是按照小端存的，所以读取的时候经常看到`memrev32ifbe(p+1)`这样的代码.
<len>后面紧跟的是对应长度的字符串表示key, 没有`\0`结尾.
key后面的跟着一个变长的<len>表示value的长度.
这个<len>后面没有紧跟value，而是有一个一个字节的<free>表示value后面的空闲的空间大小.
再后面是对应长度的value和free长度的空闲字节.
最后有一个字节表示结尾，值是255.

#### unsigned char *zipmapNew(void)
生成一个新的zm结构，包含2个字节，第一个<zmlen>是0，第二个255表示结尾.

后面都是对整个结构的decode和encode，查找key，添加/更新key, 删除key, 可以看到添加key还好，就是最后面append。对于更新key，如果原有空间不够或者更新后空闲太多，以及删除key，在这些情况下，程序需要对这个key后面的整个string进行memmove操作，这个开销应该还是不小的.

所以zipmap适合append, 或者value是int之类的固定长度的类型，这样update不会造成mommove.

### Reference:
1. [github: redis zmalloc](https://github.com/antirez/redis/blob/unstable/src/zipmap.c)
