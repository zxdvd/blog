+++
title = "代码阅读: redis-object"
date = "2018-01-13"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-object"
Authors = "Xudong"
+++

redis object对象.
robj的定义在server.h里面.

    typedef struct redisObject {
        unsigned type:4;                // 类型
        unsigned encoding:4;            // encoding类型
        unsigned lru:LRU_BITS;          // LRU LFU相关
        int refcount;                   // 引用计数(到0就释放了)
        void *ptr;                      // payload
    } robj;

### object.c

#### robj *createObject(int type, void *ptr)
根据类型创建一个robj对象, 初始encoding为`OBJ_ENCODING_RAW`, refcount置为1, 然后配置下lru.

#### robj *makeObjectShared(robj *o)
设置一个robj为shared类型(不受引用计数限制), 设置为`OBJ_SHARED_REFCOUNT`(INT_MAX)

这个函数只使用了一次，在server.c的`void createSharedObjects(void)`初始化了10000个整数

    for (j = 0; j < OBJ_SHARED_INTEGERS; j++) {
        shared.integers[j] =
            makeObjectShared(createObject(OBJ_STRING,(void*)(long)j));
        shared.integers[j]->encoding = OBJ_ENCODING_INT;
    }
注意type是`OBJ_STRING`，encoding是`OBJ_ENCODING_INT`.

### 创建RawStringObject

    robj *createRawStringObject(const char *ptr, size_t len) {
        return createObject(OBJ_STRING,sdsnewlen(ptr,len));
    }
将ptr指向的字符串复制到一个sds字符串对象里，然后以此创建一个robj.

#### robj *createEmbeddedStringObject(const char *ptr, size_t len)
创建一个内存紧跟着robj的字符串对象(适用于较小的字符串).
正常情况下robj.ptr指针指向robj实际存的东西(各种字符串等)，在内存的其它地方，这个函数在malloc的时候把robj和string的空间一起获取，这样读写的时候性能会好一些.

    robj *o = zmalloc(sizeof(robj)+sizeof(struct sdshdr8)+len+1);
    struct sdshdr8 *sh = (void*)(o+1);
    o->ptr = sh+1;

#### 创建字符串(根据长度大小创建不同类型的字符串对象)

    #define OBJ_ENCODING_EMBSTR_SIZE_LIMIT 44
    robj *createStringObject(const char *ptr, size_t len) {
        if (len <= OBJ_ENCODING_EMBSTR_SIZE_LIMIT)
            return createEmbeddedStringObject(ptr,len);
        else
            return createRawStringObject(ptr,len);
    }

#### robj *createStringObjectFromLongLong(long long value)
创建一个robj来保持`long long`类型的值, 分三种情况
1. 值范围在0-10000，直接用server.c里初始化的`shared.intergers[value]`
2. 值在long的范围内，直接`o->ptr = (void *)((long)value);`先转换成long然后以char *形式存
3. 值范围超出long了, 先转换成sds类型，然后创建robj.

#### 其它redis数据结构的封装
createQuicklistObject, createSetObject, createHashObject都是对redis支持的数据结构的封装.
后面紧跟着的是对应的free函数.

#### incrRefCount, decrRefCount
增加或减少引用次数，引用次数为1是调用decrRefCount就直接free这个robj了.
对于sharedObj, 这两个函数没有任何作用.

#### robj *tryObjectEncoding(robj *o)
这个函数对OBJ_STRING类似的, encoding为RAW和EMBSTR的robj进行重新编码，试图减少内存占用(这里没有用压缩算法).
1. 如果string比较小可以转成long存，尝试以long的形式存.
2. 尝试将比较小的RAW变成EMBSTR

#### void objectCommand(client *c)
对OBJECT这个命令的处理，可以看一个key对应的robj的引用次数，encoding，idletime等.


### Reference:
1. [github: redis anet](https://github.com/antirez/redis/blob/unstable/src/object.c)
