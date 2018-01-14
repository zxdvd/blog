+++
title = "代码阅读: redis-ziplist"
date = "2018-01-02"
categories = ["Linux"]
tags = ["read-code", "c", "redis", "ziplist"]
slug = "read-code-redis-ziplist"
Authors = "Xudong"
+++

ziplist跟zipmap非常类似，但是似乎要复杂不少，zipmap里key, value都当作字符串处理，没有类型. 但是ziplist里有int和string两大类.
ziplist实际上是个双向的链表结构.

这个主要还是看注释里的结构，代码都是对结构的解析

    * ZIPLIST OVERALL LAYOUT:
    * The general layout of the ziplist is as follows:
    * <zlbytes><zltail><zllen><entry><entry><zlend>
1. <zlbytes>是整个list的所有字符长度, 4字节uint32_t.
2. <zltail>是到最后一个entry的偏移(从头部算起)，以便于快速定位到最后一个entry(便于做pop, append操作), 4字节uint32_t
3. <zllen>占用2字节，是entry的个数，个数超过2^16-2的时候就需要遍历才能获取实际个数(跟zipmap的len类似)
4. entry是个复合结构，大致如下

    <prev_length><current_length><payload>
    1. <prev_length>是前一个entry的长度，记录这个是因为ziplist有从尾部到头部遍历的需求
    2. <current_length>是当前长度
    3. payload是实际存的内容
5. <zlend>是结束标记，值为255

#### length
再来看看length的具体表达，这个比zipmap的复杂得多
1. |00pppppp| 头两位全0，表示string, 后面6位是string的长度，最长是2^6-1=63.
2. |01pppppp|qqqqqqqq| 头两位是01，也是string，但是是2个字节表示长度，最长就是2^14-1.
3. |10______|b2|b3|b4|b5| 头两位是10, 也是string，后4个字节表示长度
4. |11000000| 1byte, 表示int16_t
5. |11010000| 1byte, 表示int32_t
6. |11100000| 1byte, 表示int64_t
7. |11110000| 1byte, 表示int24_t
8. |11111110| 1byte, 表示int8_t
9. |1111xxxx| 1byte, 这个有些复杂，后面四位就是所表达的值，后四位中`0000`, `1110`, `1111`都已经被使用了，只剩下`0001`-`1101`也就是1到13供13个值，这里把他们减1来表达0-12这13个值.

来看看判断这个length的首字节的宏，比如

    #define ZIP_INT_24B (0xc0 | 3<<4)
`0xc0 | 3<<4` = `11000000 | 11<<4` = `11000000 | 00110000` = `11110000`正好就是上面第七条的规则, 其它的都是类似.

关于length，看代码会发现prev_length, current_length的方式还不一样，current_length跟上面是一致的，但是prev_length不考虑int类型(把整个prev_entry当作一个string), 只使用了一字节和5字节的string表示方式(见函数zipPrevEncodeLength).

    #define ZIPLIST_BYTES(zl)       (*((uint32_t*)(zl)))
    #define ZIPLIST_TAIL_OFFSET(zl) (*((uint32_t*)((zl)+sizeof(uint32_t))))
    #define ZIPLIST_LENGTH(zl)      (*((uint16_t*)((zl)+sizeof(uint32_t)*2)))
    #define ZIPLIST_HEADER_SIZE     (sizeof(uint32_t)*2+sizeof(uint16_t))
    #define ZIPLIST_ENTRY_HEAD(zl)  ((zl)+ZIPLIST_HEADER_SIZE)
    #define ZIPLIST_ENTRY_TAIL(zl)  ((zl)+intrev32ifbe(ZIPLIST_TAIL_OFFSET(zl)))
这些宏分别是获取整个ziplist长度，最后一个entry的偏移，entry个数，首个entry以及末entry地址.

zipTryEncoding, zipSaveInteger, zipLoadInteger都是针对int的read, write处理.

后面都是常规的CURD操作还有merge操作.


### Reference:
1. [github: redis zmalloc](https://github.com/antirez/redis/blob/unstable/src/ziplist.c)
