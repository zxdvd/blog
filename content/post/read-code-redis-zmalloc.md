+++
title = "代码阅读: redis-zmalloc"
date = "2017-12-23"
categories = ["Linux"]
tags = ["read-code", "c", "redis", "malloc"]
slug = "read-code-redis-malloc"
Authors = "Xudong"
+++

zmalloc.*是根据编译选项，对libc的malloc, jemalloc, tcmalloc的封装.

首先说下malloc和free并不是系统调用，它们是用户态的程序，是对brk, sbrk等系统调用的封装，
让大家用的更方便，tcmalloc是google开发的替代品，jemalloc是facebook的，我没有benchmark，
但是一般都说比libc的性能更好，速度更快，而且这两个都有长时间的生产环境使用经验.

zmalloc.h这个注意是处理tcmalloc和jemalloc的编译选项，另外对于使用tcmalloc, jemalloc以及mac上的malloc，
开启了HAVE_MALLOC_SIZE, 并且定义了zmalloc_size函数(libc似乎没有这个功能, 所以作者自己封装出这个功能了)

### zmallo.c
首先有一小段宏定义`PREFIX_SIZE`, 基本就是对于tcmalloc/jemalloc，这个PREFIX_SIZE是0，
对于libc的malloc，设置这个值(对于linux，大小是`sizeof(size_t)`).
后面可以发现就是用这个来给malloc包装出malloc_size的功能.

然后后面一段宏是处理tcmalloc和jemalloc的映射. 先看看`used_memory`和`zmalloc_thread_safe`
这两个静态变量，看名字好理解，对于多线程程序，如果静态变量在多个thread里都有使用, 当然也就需要锁啦，
所以后面紧跟着定义了一个`used_meory_mutex`.

`zmalloc_default_oom`是一个out of memory的处理函数，用户可以通过修改`zmalloc_oom_handler`这个函数指针
来修改out of memory的处理逻辑.

#### zmalloc
先看看zmalloc函数
```
void *zmalloc(size_t size) {
    void *ptr = malloc(size+PREFIX_SIZE);

    if (!ptr) zmalloc_oom_handler(size);
#ifdef HAVE_MALLOC_SIZE
    update_zmalloc_stat_alloc(zmalloc_size(ptr));
    return ptr;
#else
    *((size_t*)ptr) = size;
    update_zmalloc_stat_alloc(size+PREFIX_SIZE);
    return (char*)ptr+PREFIX_SIZE;
#endif
}
```
首先malloc内存如果malloc失败，直接调用`zmalloc_oom_handler`, 省的每次malloc完都需要if检查一下.

然后，`ifdef HAVE_MALLOC_SIZE`这就是使用tcmalloc/jemalloc的时候，很简单，
执行一个update函数，直接返回指针就可以了.

对于libc的malloc，这里就有技巧了，之前通过PREFIX_SIZE想系统多要了
一点点内存，干什么的呢？保存这次malloc的内存大小，也就是size这个变量.

怎么保存呢？`*((size_t*)ptr) = size;`我尝试解读一下
        1. ptr是个void指针，通过`(size_t*)ptr`把它变成size_t的指针
        2. 对于这个size_t的指针，将它的第一个元素值设置成size

这样就实现了把malloc的size藏在头部的目的了，那么返回指针的时候自然需要把头部这几个字节给剥去，
所以有`return (char*)ptr+PREFIX_SIZE`.

#### zfree
再看zfree，跟刚刚的zmalloc对应的，就是一个update_stat和free的操作，
就是对于libc来说，之前返回给用户的时候把PREFIX_SIZE剥去了，现在用户传过来的指针需要
向前移一点，把那几个字节也一起释放啦(`realptr = (char*)ptr-PREFIX_SIZE;`)

#### update_zmalloc_stat_alloc/free
```
#define update_zmalloc_stat_alloc(__n) do { \
    size_t _n = (__n); \
    if (_n&(sizeof(long)-1)) _n += sizeof(long)-(_n&(sizeof(long)-1)); \
    if (zmalloc_thread_safe) { \
        atomicIncr(used_memory,__n,used_memory_mutex); \
    } else { \
        used_memory += _n; \
    } \
} while(0)
```
第一个if看着很复杂，就一个目的，把__n转换成8的倍数. 因为malloc分配
内存的时候会向上去sizeof(long)的倍数.

后面才是这个函数的主要目的, `used_memory += _n;`. 只不过因为`used_memory`是共享变量，需要保证`+=`操作的原子性.

`update_zmalloc_stat_free`做的是`-=`操作.

#### zmalloc_size
这个是针对libc的malloc专门的一个函数，就是对于传过来的指针，向前找PREFIX_SIZE个字节，取出size返回.

#### zstrdup
字符串复制函数，使用前面定义的zmalloc

#### zcalloc, zrealloc
跟zmalloc差不多, 对calloc, realloc的封装.

#### zmalloc_get_rss
RSS(Resident Set Size), 程序实际占用的内存, 不含swap.

这个函数根据系统的情况，有3种计算方式，linux是从/proc/pid/stat, 里面去的，
第二十四个field是rss的页数吧，然后乘以页的大小.

mac系统有系统函数直接可以获取，其他的系统直接返回上面统计的`used_memory`.

#### zmalloc_get_smap_bytes_by_field
这个函数是从`/proc/self/smaps`读取特定field的值然后求和. 这个文件存的是进程里地址空间的详情，
比如某个库用了多少内存，具体的rss, shared, clean, dirty等数值.
这个函数可以接受一个参数，对对应的field的值求和的.

对于没有proc的系统，这个函数直接返回0了.

#### zmalloc_get_memory_size
这个函数是获取系统的内存大小，这里应该是用到了各个系统一些比较tricky的做法，
这里主要是做了各种不同的操作系统的兼容处理.

### Reference:
1. [github: redis zmalloc](https://github.com/antirez/redis/blob/unstable/src/zmalloc.c)
