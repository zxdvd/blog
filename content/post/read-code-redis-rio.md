+++
title = "代码阅读: redis-rio"
date = "2017-12-20"
categories = ["Linux"]
tags = ["read-code", "c", "redis", "rio"]
slug = "read-code-redis-rio"
Authors = "Xudong"
+++

rio.h, rio.c是redis内部对部分io的一个封装，封装之后不管是buffer, 还是file, 还是fdset，
它们都是rio对象，有统一的write， read等函数，外部调用它们的时候不用关心
底层细节.
从这里可以学到c里面怎么来实现面向对象编程，我们会发现面向对象只是一种思想，
跟语言无关，并不需要class关键词.

先看rio.h里rio的这个结构的定义(篇幅限制，只copy部分)
```
struct _rio {
    size_t (*read)(struct _rio *, void *buf, size_t len);
    size_t (*write)(struct _rio *, const void *buf, size_t len);
    off_t (*tell)(struct _rio *);
    int (*flush)(struct _rio *);

    size_t processed_bytes;  /* number of bytes read or written */
    size_t max_processing_chunk;  /* maximum single read or write chunk size */

    union {
        struct {
            sds ptr;
            off_t pos;
        } buffer;
        struct {
            FILE *fp;
            off_t buffered; /* Bytes written since last fsync. */
            off_t autosync; /* fsync after 'autosync' bytes written. */
        } file;
    } io;
};
```
首先定义了read, write, tell, flush四种很常见的函数指针，方便后面初始化的
时候指向对应函数. 而且注意这些函数的参数都是抽象的`rio *`.

union里面有buffer, file, fdset三种io类型，后面其实就是基于这三种类型派生出3个对象.

rio.h后面的几个函数rioWrite, rioRead, rioTell, rioFlush是rio对象的通用方法，它们是对rio对象的read, write, tell, flush的小程度封装，这里都不用关心底层具体是那种io类型.

### rio.c
首先以buffer为例，定义了rioBufferWrite, rioBufferRead, rioBufferTell, rioBufferFlush
这些针对buffer的底层write, read, tell, flush实现.
然后派生出一个rio的buffer示例rioBufferIO, 用上面这些函数初始化rio的函数指针.

后面的file，fdset都是类似与这样的。

这样整个定义完了，对外暴露统一的read, write, tell, flush,
后面的一堆rioWriteBulkXXXX函数就不需要关心底层细节了.

关于rio.*的部分，代码并不难理解，我们需要好好学习封装和抽象的艺术.

### Reference:
1. [github: redis](https://github.com/antirez/redis/blob/unstable/src/rio.c)
