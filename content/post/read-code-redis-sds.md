+++
title = "代码阅读: redis-sds"
date = "2017-12-28"
categories = ["Linux"]
tags = ["read-code", "c", "redis", "sds"]
slug = "read-code-redis-sds"
Authors = "Xudong"
+++

sds是一个动态字符串的模块.

### sds.h

        typedef char *sds;
很多代码里类似的定义，`typedef char *string;`, 把string定义成char指针.

        struct __attribute__ ((__packed__)) sdshdr16 {
            uint16_t len; /* used */
            uint16_t alloc; /* excluding the header and null terminator */
            unsigned char flags; /* 3 lsb of type, 5 unused bits */
            char buf[];
        };
__attribute__是编译器提供的关键词，后面的`__packed__`是它的一种属性之一，
表示这个struct不需要为了align而进行padding，关于struct的padding我的另一个
笔记里写过，这里就不说了.
这个struct有4个元素，len是长度，alloc是分配的空间，flags是类型区分，
buf[]是真正的payload数据. alloc和len是不一样的，len可能小于alloc，因为分配的空间不一定被使用了.

这样的定义有sdshdr5, sdshdr8, sdshdr16, sdshdr32, sdshdr64共五种，
后面的数字n表示字符串最大长度，比如16就是2^16-1, 所以len的类型是uint16_t.

每个里面的flags字段都一样是8bit，但是只使用了低3位，因为一共就五种类型，
低三位就足够了, 0表示sdshdr5, 1表示sdshdr8....

仔细看会发现sdshdr5有一些特殊，没有len和alloc，那么怎么表示长度呢？
sdshdr5的最大长度为2^5-1, 只需要5位表示，还记得flags的8位里只用了
低三位对吧，在sdshdr5里flags的高五位就正好用来表示长度了.



    #define SDS_TYPE_BITS 3
    #define SDS_TYPE_5_LEN(f) ((f)>>SDS_TYPE_BITS)
这里如果写成`#define SDS_TYPE_5_LEN(flags) ((flags)>>3)`可能就
更容易看出来了，这就是对于sdshdr5来说，把flags右移三位，去掉了低三位，
剩下的高五位就正好是这个sds的长度了.

    #define SDS_HDR(T,s) ((struct sdshdr##T *)((s)-(sizeof(struct sdshdr##T))))
先看看后面的用法，都是这样的SDS_HDR(16, s), 代进去就是

    #define SDS_HDR(16,s) ((struct sdshdr16 *)((s)-(sizeof(struct sdshdr16))))
这里的s实际上sdshdrXX的payload的其实地址，也就是buf[]. 那么(s-sizeof(struct sdshdrXX))
实际上就是这个sdshdrXX的起始地址了，这里跟zmalloc的那个PREFIX_SIZE道理差不多.
然后通过(struct sdshdrXX *)显式转换一下，然后就可以`SDS_HDR(16, s)->len`这样用了.

    #define SDS_HDR_VAR(T,s) struct sdshdr##T *sh = (void*)((s)-(sizeof(struct sdshdr##T)));
跟上面一样的，不过这里将sh指向sdshdrXX的起始地址了，然后可以sh->len, sh->alloc这样使用了.

#### static inline size_t sdslen(const sds s)

    static inline size_t sdslen(const sds s) {
        unsigned char flags = s[-1];
        switch(flags&SDS_TYPE_MASK) {
            case SDS_TYPE_5:
                return SDS_TYPE_5_LEN(flags);
            case SDS_TYPE_8:
                return SDS_HDR(8,s)->len;
        // 后面几个类似的省略了
        }
        return 0;
    }
之前说了s是指向sdshdrXX的buf[]的，在所有5种类型李buf[]前面都是
占一个byte的flags成员，所以`s[-1]`就是flags. 获得了flags就知道到底是sdshdr5还是sdshdr16等等.
然后调用SDS_HDR(XX, s)->len返回的就是字符串长度了.

#### static inline size_t sdsavail(const sds s)
这个跟sdslen类似，获取到具体的类型后，sh->alloc - sh->len，就是还剩下多少未使用的空间.

#### static inline void sdssetlen(sds s, size_t newlen)
这里是设置新的len值，注意这里没有校验alloc大小.

#### static inline void sdsinclen(sds s, size_t inc)
增加s的长度

#### static inline size_t sdsalloc(const sds s)
获取s的alloc大小

#### static inline void sdssetalloc(sds s, size_t newlen)
设置新的alloc大小

### sds.c

#### static inline int sdsHdrSize(char type)
获取sdshdrXX的struct大小

#### static inline char sdsReqType(size_t string_size)
根据string_size大小来确定用那一种sdshdrXX，比如小于2^5的就用sdshdr5, 大于2^5小于2^8的用sdshdr8....

#### sdsnewlen
    sds sdsnewlen(const void *init, size_t initlen) {
    void *sh;
    sds s;
    // 根据起始长度可以确定sdshdrXX的具体类型
    char type = sdsReqType(initlen);
    // 根据类型获取该struct的size
    int hdrlen = sdsHdrSize(type);
    unsigned char *fp; /* flags pointer. */

    // malloc内存, +1是因为最后需要`\0`, s_malloc就是zmalloc(定义在sdsalloc.h)
    sh = s_malloc(hdrlen+initlen+1);
    // 如果没有提供初始化字符串，则全部设置为0
    if (!init)
        memset(sh, 0, hdrlen+initlen+1);
    if (sh == NULL) return NULL;
    // sh右移hdrlen个byte就是buf[]的起始位置，把s指向那里
    s = (char*)sh+hdrlen;
    // s前一个byte是flags
    fp = ((unsigned char*)s)-1;
    switch(type) {
        case SDS_TYPE_5: {
            *fp = type | (initlen << SDS_TYPE_BITS);
            break;
        }
        case SDS_TYPE_8: {
            SDS_HDR_VAR(8,s);
            sh->len = initlen;
            sh->alloc = initlen;
            *fp = type;
            break;
        }
    }
    // 如果提供了初始化字符串，则memcpy一下
    if (initlen && init)
        memcpy(s, init, initlen);
    // 设置'\0'结尾
    s[initlen] = '\0';
    // 注意返回的s是剥去了hdr的部分，是buf[]的起始位置
    return s;
}

#### sds sdsnew(const char *init)
通过strlen获取字符串长度，然后调用sdsnewlen

#### void sdsfree(sds s)

    if (s == NULL) return;
    s_free((char*)s-sdsHdrSize(s[-1]));
s[-1]是flags，sdsHdrSize(s[-1])是根据flags可以知道type，然后得到
hdr的size，`(char *)s - hdrSize`就是该struct的真正的起始位置，然后直接free掉就可以了.

#### void sdsupdatelen(sds s)

    int reallen = strlen(s);
    sdssetlen(s, reallen);
因为s的len不一定真的是s这个字符串的实际长度(比如s本来是100个字符，突然把第50个字符设置成'\0', 那么长度只有49了). 这个函数用s字符串的真实长度来更新s的len.

#### sds sdsMakeRoomFor(sds s, size_t addlen)
先判断下当前的`alloc - len`是不是大于addlen，如果是说明可用空间够，直接返回s就可以.
如果不够，newlen = min(2*(len+addlen), newlen+1MB), 这样是提前多malloc一些，留一些余量，避免频繁realloc/malloc

    // 根据新的newlen获取恰当的type, 根据type获取hdrlen
    type = sdsReqType(newlen);
    hdrlen = sdsHdrSize(type);
    if (oldtype==type) {
        // 如果新的类型跟旧类型一致，hdr大小就不变了，buf[]就没有偏移，只需要realloc就可以
        newsh = s_realloc(sh, hdrlen+newlen+1);
        if (newsh == NULL) return NULL;
        s = (char*)newsh+hdrlen;
    } else {
        // 如果type变了，hdr大小必然改变, 导致buf[]需要想后面偏移，干脆就malloc然后再memcpy
        newsh = s_malloc(hdrlen+newlen+1);
        if (newsh == NULL) return NULL;
        memcpy((char*)newsh+hdrlen, s, len+1);
        s_free(sh);
        // 返回的s是buf[]的起始位置，需要加上hdr的size大小
        s = (char*)newsh+hdrlen;
        s[-1] = type;
        sdssetlen(s, len);
    }
    sdssetalloc(s, newlen);
    return s;
注意这个函数可能修改了s的地址(如果使用了malloc).

#### sds sdsRemoveFreeSpace(sds s)
这里跟sdsMakeRoomFor很类似，不过是减小s占用的内存.
通过s的实际的len获得恰当的type，跟当前的type比较来确定是realloc还是重新malloc, memcpy, free.

#### size_t sdsAllocSize(sds s)
获取s实际占用的内存.
这里我感觉也可以直接用zmalloc的zmalloc_size函数.

#### void *sdsAllocPtr(sds s)
返回sdshdrXX的起始位置(s只是buf[]的指针而不是hrd的)

#### void sdsIncrLen(sds s, int incr)
这里是修改s的len，主要是针对不同类型用assert进行了边界检查，最后设置s[len] = '\0'

#### sds sdscatlen(sds s, const void *t, size_t len)
这是个字符串拼接函数，把t接在s后面.
注意因为有拼接，需要调用sdsMakeRoomFor, 所以s的地址可能会改变，因此函数返回了s的地址.

#### sds sdscpylen(sds s, const char *t, size_t len)
将长度为t的字符串复制到s里面.
如果s的alloc大小不够，则需要调用sdsMakeRoomFor来扩大.

#### int sdsll2str(char *s, long long value)
这是一个long long类型的数字转换成字符串形势的通用函数

#### sds sdscatfmt(sds s, char const *fmt, ...)
这个是一个类似于sprintf的实现，不过只支持%s, %S, %i, %I, %u, %U, %%，而且这些格式是自己定义的.
作者认为这个简单的实现比sprintf快很多.

#### sds sdstrim(sds s, const char *cset)
字符串头部和尾部trim，把有cset里面的字符组成的连续的串给去掉.

#### void sdsrange(sds s, int start, int end)
这个函数将s这个字符串改成s中的某一段，注意这里用memmove直接修改了原始字符串的.
`s[newlen] = 0;`等价于`s[newlen] = '\0';`(\0的二进制值就是0x00), 我觉得代码里统一写法比较好.

#### sdstolower, sdstoupper, sdscmp
常用的操作函数, 注意sdscmp里用的是memcmp而不是strcmp, 因为sds串中间是可以有'\0'的.

#### sds *sdssplitlen(const char *s, int len, const char *sep, int seplen, int *count)

#### sds sdscatrepr(sds s, const char *p, size_t len)
将字符串p拼接在s后面，把\n这样的non-printable的字符转换成`\\n`这样的两个字符(`\\`转义后就是'\').

#### is_hex_digit, hex_digit_to_int
digit字符和int的转换

#### sds sdsmapchars(sds s, const char *from, const char *to, size_t setlen)
类似于tr的字符串替换

#### sds sdsjoin(char **argv, int argc, char *sep)
字符串数组join的函数


### Reference:
1. [github: redis sds.h](https://github.com/antirez/redis/blob/unstable/src/sds.h)
2. [github: redis sds.c](https://github.com/antirez/redis/blob/unstable/src/sds.c)
3. [gcc doc: __attribute__](https://gcc.gnu.org/onlinedocs/gcc-4.0.2/gcc/Type-Attributes.html)
