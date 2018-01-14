+++
title = "代码阅读: redis-eventloop"
date = "2017-12-29"
categories = ["Linux"]
tags = ["read-code", "c", "redis", "eventloop"]
slug = "read-code-redis-eventloop"
Authors = "Xudong"
+++

ae*相关的文件是redis对各个系统的event drive模块的封装，功能跟libevent,
libuv差不多，
目前支持select, poll, epoll, kqueue, evport这些.

### ae.h
先定义了AE_NONE, AE_READABLE, AE_WRITABLE三种事件读写属性, 以及AE_FILE_EVENTS, AE_TIME_EVENTS
两种事件类型(AE_DONT_WAIT是属性), 这些都是可以进行或操作叠加的，比如一个事件可以同时注册读写属性.

#### aeFileEvent

    typedef struct aeFileEvent {
        int mask; /* one of AE_(READABLE|WRITABLE) */
        aeFileProc *rfileProc;
        aeFileProc *wfileProc;
        void *clientData;
    } aeFileEvent;
mask是事件读写属性掩码.
aeFileProc是一个函数类型，rfileProc, wfileProc都是这个类型的函数指针, 这里如果不先定义一下就得这么写了

    void (*rfileProc)(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);

#### aeTimeEvent

    typede aeTimeEvent {
        long long id; /* time event identifier. */
        long when_sec; /* seconds */
        long when_ms; /* milliseconds */
        aeTimeProc *timeProc;
        aeEventFinalizerProc *finalizerProc;
        void *clientData;
        struct aeTimeEvent *next;
    } aeTimeEvent;
id是事件id, when_sec, when_ms组成一个时间, timeProc, finalizerProc是事件处理的函数指针.

#### aeEventLoop

    typede aeEventLoop {
        int maxfd;   /* highest file descriptor currently registered */
        int setsize; /* max number of file descriptors tracked */
        long long timeEventNextId;
        time_t lastTime;     /* Used to detect system clock skew */
        aeFileEvent *events; /* Registered events */
        aeFiredEvent *fired; /* Fired events */
        aeTimeEvent *timeEventHead;
        int stop;
        void *apidata; /* This is used for polling API specific data */
        aeBeforeSleepProc *beforesleep;
    } aeEventLoop;
aeEventLoop包含fileEvent和timeEvent.

### ae.c
这里是对select, epoll, kqueue的封装.

#### order

    #ifdef HAVE_EVPORT
    #include "ae_evport.c"
    #else
        #ifdef HAVE_EPOLL
        #include "ae_epoll.c"
        #else
            #ifdef HAVE_KQUEUE
            #include "ae_kqueue.c"
            #else
            #include "ae_select.c"
            #endif
        #endif
    #endif
ae_evport.c, ae_epoll.c, ae_kqueue.c, ae_select.c是对底层multiplexing的封装.
如果是solaris系统，就使用port, 其次是epoll，再次是kqueue，最后是select作为fallback.

#### aeEventLoop *aeCreateEventLoop(int setsize)
创建一个aeEventLoop对象并进行初始化, 调用ae_XXX.c里的aeApiCreate进行底层处理.

初始化的时候event的mask被置为了AE_NONE.

#### int aeResizeSetSize(aeEventLoop *eventLoop, int setsize)
这个是对eventLoop进行扩容(只能扩大，不能减小), 主要是调用ae_XXX.c的aeApiResize函数.
调用成功后，对events, fired进行realloc, 并初始化新增的这些events.

#### int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask, aeFileProc *proc, void *clientData)
这里是向eventLoop里添加一个fileEvent事件，设置对应的读写mask, 并更新maxfd.

#### void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask)
这里是删除一个事件的读/写/读写, 注意这里可能只是修改某个事件注册的类型，
比如原来是读写，现在去掉写的监听，只监听读.

#### int aeGetFileEvents(aeEventLoop *eventLoop, intfd)
获取事件的读写属性.

#### static void aeGetTime(long *seconds, long *milliseconds)
获取当前时间戳，并转换成秒加上毫秒的形势.

#### static void aeAddMillisecondsToNow(long long milliseconds, long *sec, long *ms)
函数名意思就非常明白了.

#### long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds, aeTimeProc *proc, void *clientData, aeEventFinalizerProc *finalizerProc)
初始化一个若干毫秒后触发的timeEvent事件并插入eventLoop的时间事件链表的头部.

#### int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id)
根据id删除一个timeEvent，起始这里没有删除，只是把id标记为-1了.

#### static aeTimeEvent *aeSearchNearestTimer(aeEventLoop *eventLoop)
从timeEvent的链表里找最近的时间(时间最小的)

#### static int processTimeEvents(aeEventLoop *eventLoop)
这里是处理时间事件的，前面有一段代码是修正系统时间漂移的.
首先处理的是在aeDeleteTimeEvent里伪删除的哪些events，调用他们的finalizerProc函数，然后真正的删除它们.

然后执行那些已经到时间的任务.
返回的是执行的任务的个数(不包括真删除的那部分)

#### int aeProcessEvents(aeEventLoop *eventLoop, int flags)
根据flags的情况决定处理什么类型的events.
1. 如果flags包含timeEvent
    1.1 获取最近一个时间快要到的timeEvent
    1.2 计算跟当前时间的时间间隔是多少ms, 转换成struct timeval结构存入tvp.
2. 如果flags不包含timeEvent
    2.1 如果AE_DONT_WAIT开启了, 设置tvp为0(非阻塞), 否则设置为NULL(阻塞)
3. `numevents = aeApiPoll(eventLoop, tvp);`
    3.1 这里调用select/epoll等, 根据tvp的值决定是非阻塞还是阻塞，以及阻塞时间.
    3.2 处理poll的结果.
4. 如果AE_TIME_EVENTS开启了, 调用processTimeEvents(eventLoop)

#### int aeWait(int fd, int mask, long long milliseconds)
这个函数跟之前的都不太一样，之前的那些是对select/poll的跨平台封装，aeWait是用poll来监听一个
文件是否可读/可写.

#### void aeMain(aeEventLoop *eventLoop)

    eventLoop->stop = 0;
    while (!eventLoop->stop) {
        if (eventLoop->beforesleep != NULL)
            eventLoop->beforesleep(eventLoop);
        aeProcessEvents(eventLoop, AE_ALL_EVENTS);
    }
只要eventLoop没有stop，就一直循环处理各种events.

在ae.c里面的各种crud操作里实际调用的是底层封装的aeApiCreate, aeApiResize, aeApiFree, aeApiAddEvent, aeApiDelEvent, aeApiPoll这些函数，下面简单看看select/epoll里的实现，其它平台就忽略了.

### ae_select.c

    typedef struct aeApiState {
        fd_set rfds, wfds;
        fd_set _rfds, _wfds;
    } aeApiState;
select的api如下，nfds是最大的fd加1, readfds, writefds, exceptfds是需要监测的队列.

    int select(int nfds, fd_set *readfds, fd_set *writefds,
               fd_set *exceptfds, struct timeval *timeout);
我们只关注读写，那么需要有地方存readfds, writefds, 而且select会修改传过去的readfds, writefds，
那么还需要有地方存一份副本, 这就是aeApiState的结构由来啦.

#### static int aeApiCreate(aeEventLoop *eventLoop)
select的eventloop创建, 初始化aeApiState结构，`FD_ZERO(&state-rfds);`将readfds, writefds置空.

#### static int aeApiResize(aeEventLoop *eventLoop, int setsize)
这里只是检查setsize是否超过了`FD_SETSIZE`(select的限制, 1024)

#### static int aeApiAddEvent(aeEventLoop *eventLoop, int fd, int mask)

    aeApiState *state = eventLoop->apidata;

    if (mask & AE_READABLE) FD_SET(fd,&state->rfds);
    if (mask & AE_WRITABLE) FD_SET(fd,&state->wfds);
    return 0;
使用FD_SET将fd添加对对应的队列.

#### static int aeApiDelEvent(aeEventLoop *eventLoop, int fd, int mask)
同理，使用FD_CLR清除出队列.

#### static int aeApiPoll(aeEventLoop *eventLoop, struct timeval *tvp)

    // 拷贝rfds, wfds到副本然后传给select
    memcpy(&state->_rfds,&state->rfds,sizeof(fd_set));
    memcpy(&state->_wfds,&state->wfds,sizeof(fd_set));

    retval = select(eventLoop->maxfd+1,
                &state->_rfds,&state->_wfds,NULL,tvp);
    if (retval > 0) {
        // 轮询eventLoop里的events数组
        for (j = 0; j <= eventLoop->maxfd; j++) {
            int mask = 0;
            aeFileEvent *fe = &eventLoop->events[j];

            if (fe->mask == AE_NONE) continue;
            if (fe->mask & AE_READABLE && FD_ISSET(j,&state->_rfds))
                mask |= AE_READABLE;
            if (fe->mask & AE_WRITABLE && FD_ISSET(j,&state->_wfds))
                mask |= AE_WRITABLE;
            // 只要该fileEvent在select返回的数组里，添加到eventLoop的fired数组里
            eventLoop->fired[numevents].fd = j;
            eventLoop->fired[numevents].mask = mask;
            numevents++;
        }
    }
这里是select的poll处理，主要就是监听并将结果放入到eventLoop的fired里供后面processEvents的时候处理.

### ae_epoll.c

    typedef struct aeApiState {
        int epfd;
        struct epoll_event *events;
    } aeApiState;
epoll需要一个epoll实例描述符(存epoll_create()的返回值),
还需要epoll_event指针用来接受epoll_wait返回的事件.

#### static int aeApiCreate(aeEventLoop *eventLoop)

    aeApiState *state = zmalloc(sizeof(aeApiState));
    state->epfd = epoll_create(1024);   // 实例化

#### static int aeApiResize(aeEventLoop *eventLoop, int setsize)

    state->events = zrealloc(state->events, sizeof(struct epoll_event)*setsize);

#### static int aeApiAddEvent(aeEventLoop *eventLoop, int fd, int mask)

    epoll_ctl(state->epfd,op,fd,&ee);
通过epoll_ctl添加或者修改event.

#### static int aeApiPoll(aeEventLoop *eventLoop, struct timeval *tvp)

    retval = epoll_wait(state->epfd,state->events,eventLoop->setsize,
            tvp ? (tvp->tv_sec*1000 + tvp->tv_usec/1000) : -1);
    if (retval > 0) {
        int j;

        numevents = retval;
        // 这里numevents是指返回的有事件触发的数量
        // 具体接受到的事件在state->events里, 遍历该指针, 将对应的事件写入eventLoop->fired数组里
        for (j = 0; j < numevents; j++) {
            int mask = 0;
            struct epoll_event *e = state->events+j;

            if (e->events & EPOLLIN) mask |= AE_READABLE;
            if (e->events & EPOLLOUT) mask |= AE_WRITABLE;
            if (e->events & EPOLLERR) mask |= AE_WRITABLE;
            if (e->events & EPOLLHUP) mask |= AE_WRITABLE;
            eventLoop->fired[j].fd = e->data.fd;
            eventLoop->fired[j].mask = mask;
        }
    }

看了下ae_kqueue.c发现kqueue跟epoll的api非常接近. 结合select和epoll的对比分析下差异和优缺点
1. select受限于FD_SETSIZE的大小(默认1024), 能监听的fd太少, epoll的maxevents(第三个参数)是个int类型
2. select的rfds, wfds是值传递，返回的时候作为变化的事件列表返回，因此传进去前要保存原始数据(memcpy)
3. 因为上面第二点的原因，自然内存开销大一些，性能也会差一些
4. epoll是同一个epoll_event上可以开启多个事件类型, select是分read, write, exception3个列表.
5. select只支持水平触发，epoll支持边沿触发和水平触发


### Reference:
1. [github: redis ae.h](https://github.com/antirez/redis/blob/unstable/src/ae.h)
2. [github: redis ae.c](https://github.com/antirez/redis/blob/unstable/src/ae.c)
2. [github: redis ae_epoll.c](https://github.com/antirez/redis/blob/unstable/src/ae_epoll.c)
2. [github: redis ae_select.c](https://github.com/antirez/redis/blob/unstable/src/ae_select.c)
