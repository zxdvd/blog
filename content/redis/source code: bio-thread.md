<!---
title: 代码阅读: redis-thread
date: 2017-12-16
tags: read-code, c, redis, thread
slug: read-code-redis-thread
-->

大家都知道redis是单线程的，没有锁，非常快，但是不能充分利用多核，实际上redis的内部实现
是有多线程的，这篇单独拿出来讲一讲。

我们搜下包含`pthread_`的代码，发现就4个文件, atomicvar.h, bio.c, lazyfree.c, zmalloc.c,
绝大部分都在bio.c里面, 而且目前目的很单一，就是3个线程分别处理3种不同类型的job.

在bio.h里面定义了3种类型的job
```
#define BIO_CLOSE_FILE    0 /* Deferred close(2) syscall. */
#define BIO_AOF_FSYNC     1 /* Deferred AOF fsync. */
#define BIO_LAZY_FREE     2 /* Deferred objects freeing. */
```

bio.c首先定义了一组静态变量，每个job一个，分别是线程，互斥锁，信号量，存job的双向链表.

### 线程创建
把线程创建的代码提取出来大致如下
```
pthread_attr_t attr;
pthread_t thread;

pthread_attr_init(&attr);
if (pthread_create(&thread,&attr,bioProcessBackgroundJobs,arg) != 0) {
        exit(1);
}
```
就是初始化一个`pthread_attr_t`的attr，然后`pthread_create`就可以，第三个参数是函数入口，在我们这里是bioProcessBackgroundJobs.

### stacksize
在创建线程中我忽略了关于stacksize的那部分，因为不是必须的，那里的逻辑是如果默认的stacksize
小于4M则将stacksize扩大到4M。对于linux系统默认stacksize可以通过`ulimit -a`里的stack size项来看，或者直接`ulimit -s`.

### thread cancel
```
pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
```
这个是设置该线程可以随时被cancel的，第一个是默认设置，理论上不需要写，第二个是让
线程收到cancel信号立即cancel，因为为了避免一个关键的执行被突然打断，系统是有一些
默认的cancel point, 比如accept, close, connect, poll, select等，在这些地方去cancel的, 设置为ASYNC就随时cancel了.

man文档和TLPI书里都说ASYNC是rarely used, 但是这里这么用是因为这些线程正常情况是不会被cancel的，
只有在redis crash的时候才调用bioKillThreads，所以这么用的.

### kill thread
```
if (pthread_cancel(bio_threads[j]) == 0) {
    if ((err = pthread_join(bio_threads[j],NULL)) != 0) {
        serverLog(LL_WARNING,
            "Bio thread for job type #%d can be joined: %s",
                j, strerror(err));
    } else {
        serverLog(LL_WARNING,
            "Bio thread for job type #%d terminated",j);
    }
}
```
如果cancel成功了就join(不join会产生僵尸线程)

### thread cleanup
```
void pthread_cleanup_push(void (*routine)(void*), void *arg);
void pthread_cleanup_pop(int execute);
```
给thread添加atExit handler，redis没有用到这两个函数.


### block signal
```
sigset_t sigset;
sigemptyset(&sigset);
sigaddset(&sigset, SIGALRM);
if (pthread_sigmask(SIG_BLOCK, &sigset, NULL)) { ... }
```
这里是block SIGALRM这个signal，确保这个signal由主线程处理.


### pthread_mutex_t
当需要对多个线程共享的变量进行操作时就需要用到锁，如果都是读取的话，可以不需要，但是哪怕是一个写另一个读，读的那一方
还是需要锁(除非写是原子的), 设想一个线程对一个thread的a, b进行写，另一个读，如果只写了a,b还没有开始写，另一个读取了整个的a,b就会出现不一致的情况.

这里`bioCreateBackgroundJob`对存储job的双向链表进行append的动作，在生成job的时候不需要mutex，但是添加的过程是在mutex里面的.

### condition variable
condition variable也是线程同步的方法之一，而且都是配合mutex一起使用的.
```
if (listLength(bio_jobs[type]) == 0) {
    pthread_cond_wait(&bio_newjob_cond[type],&bio_mutex[type]);
    continue;
}
```
当job链表的长度为0时，就sleep等待condidtion variable信号，CV发生变化内核会重新调度这个线程.

那么什么时候CV信号来了呢？当然是有新的任务产生的时候, 在`bioCreateBackgroundJob`函数里
```
pthread_cond_signal(&bio_newjob_cond[type]);
```

有一点疑惑的地方是，我们可以看到pthread_cond_wait之前，mutex被lock了，但是创建的时候需要先获取mutex锁，
按理获取不到的，那怎么可能产生新的job呢？

如果仔细看pthread_cond_wait，可以发现它实际上是一个`unlock, block, lock`的操作，当它陷入sleep的时候，实际上
是隐式的释放了锁，而信号到了的时候实际上有个隐式的上锁过程，这也就是为啥第二个参数是一个mutex. 如果有多个
线程同时等待的话，那么就只有一个能获得成功.

### process job
job的处理就是根据不同的job类型，做不同的处理，比如关闭文件，aof_fsync, lazy_free.
可以发现为了让主线程尽可能快的响应请求，这些稍慢的阻塞的活都放在专门的job线程处理了，
最后处理完了就free job.

### summary
总结下，redis一直被认为是单线程的是因为它确实没有很复杂的多线程运行，
几乎大部分的事都是主线程处理的，就上面的几个job处理是多线程的，而且可以认为是非核心的东西.

通过redis对多线程的使用，可以发现在是否使用多进程/多线程的时候需要好好
想想是否真的有必要，多线程会带来大量锁和信号量的开销，以及竟态等问题，
提高了编程的复杂度，debug难度也大一些。我们可以考虑像redis这样把系统
尽可能的拆分成多个小的子系统，尽可能减小子系统间的过多的交集，然后
每个子系统用独立的线程处理，这样有点类似于web开发里所谓的微服务架构，
redis里面的job的双向链表就类似于消息队列了.

### Reference:
1. [github: redis](https://github.com/antirez/redis/blob/unstable/src/bio.c)
