<!---
tags: redis, eventloop
-->

## redis: eventloop

Asynchronous event processing system is very important to high throughput network
 applications. However, different operating systems have different event api. Solaris
 has port, bsd related including macos uses kqueue while linux has epoll. And they
 all support basic select api.

It's better to have a uniform event driven api. You can choose libuv or libevent.
 While the author implemented a lightweight one by himself.

The `ae` module provides a group of high-level uniform apis to deal with events. It
 tries to use best low level apis on different OS. And this is also a nice example
 of OOP with C.

```c
// ae.c
/* Include the best multiplexing layer supported by this system.
 * The following should be ordered by performances, descending. */
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
```

### eventloop and event definition
Each event has a `mask` to express event types it will triggered, and two function
 pointers as read callback and write callback.
You may wonder that why it doesn't have a file descriptor member. Thus because the
 eventloop stores all events using an array. And it uses the file descriptor as array
 index. So no need to store it in each event.

An `aeFileEvent` means a event that polled from select/epoll/port. Then we can read or
 write from it. It has a fd (array index of events) field and a mask field.

```c
typedef struct aeFileEvent {
    int mask; /* one of AE_(READABLE|WRITABLE|BARRIER) */
    aeFileProc *rfileProc;
    aeFileProc *wfileProc;
    void *clientData;
} aeFileEvent;

typedef struct aeFiredEvent {
    int fd;
    int mask;
} aeFiredEvent;
```

The `aeEventLoop` is the core structure of the eventloop. The `setsize` is the size
 of the `events`. The `fired` stores all polled events that were ready to read or
 write. The `beforesleep` and `aftersleep` are two hooks that executed before and
 after a poll.

The `apidata` points to underground polling system data structure.

```c
typedef struct aeEventLoop {
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
    aeBeforeSleepProc *aftersleep;
    int flags;
} aeEventLoop;
```

### create eventloop
The `aeCreateEventLoop` function receives a parameter to set the event size. It
 initializes each members and calls the low level `aeApiCreate` to setup the event
 poll system.

```c
// core part of aeCreateEventLoop

    eventLoop->events = zmalloc(sizeof(aeFileEvent)*setsize);
    eventLoop->fired = zmalloc(sizeof(aeFiredEvent)*setsize);
    if (eventLoop->events == NULL || eventLoop->fired == NULL) goto err;
    eventLoop->setsize = setsize;

    if (aeApiCreate(eventLoop) == -1) goto err;
    /* Events with mask == AE_NONE are not set. So let's initialize the
     * vector with it. */
    for (i = 0; i < setsize; i++)
        eventLoop->events[i].mask = AE_NONE;
```

### resize
The `aeResizeSetSize` can resize a eventloop to a new size. It can be larger or smaller.
If it is resized to a smaller one, it must not be small than the `eventLoop->maxfd`.
This functions just reallocs `events` and `fired` and initializes new events.

### add/remove an event to eventloop
The function `aeCreateFileEvent` will add a new fd, related mask and callback functions
 to the eventloop.

```c
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
        aeFileProc *proc, void *clientData)
{
    if (fd >= eventLoop->setsize) {
        errno = ERANGE;
        return AE_ERR;
    }
    aeFileEvent *fe = &eventLoop->events[fd];

    if (aeApiAddEvent(eventLoop, fd, mask) == -1)
        return AE_ERR;
    fe->mask |= mask;
    if (mask & AE_READABLE) fe->rfileProc = proc;
    if (mask & AE_WRITABLE) fe->wfileProc = proc;
    fe->clientData = clientData;
    if (fd > eventLoop->maxfd)
        eventLoop->maxfd = fd;
    return AE_OK;
}
```

For remove an event, attention that you can remove read or write from an event if you
 want. For example, you can add an event with read and write mask, then you can remove
 only read mask, the event will be still in the eventloop but with only write mask.

### process events
The `aeProcessEvents` function will poll events from eventloop and process them.
It will calculate a timeout for the polling. However, if the eventloop was flagged with
 `AE_DONT_WAIT`, the timeout will be set to 0 which means returning if no events were
 ready.

Normally it will execute read callback first and them the write callback. But you can
 set the `AE_BARRIER` flag to invert this order.

```c
int aeProcessEvents(aeEventLoop *eventLoop, int flags) {
    ...
    // set timtout to 0 with flagged with AE_DONT_WAIT
        if (eventLoop->flags & AE_DONT_WAIT) {
            tv.tv_sec = tv.tv_usec = 0;
            tvp = &tv;
        }
        // poll events
        numevents = aeApiPoll(eventLoop, tvp);

        /* After sleep callback. */
        if (eventLoop->aftersleep != NULL && flags & AE_CALL_AFTER_SLEEP)
            eventLoop->aftersleep(eventLoop);

        // process all polled events
        for (j = 0; j < numevents; j++) {
            aeFileEvent *fe = &eventLoop->events[eventLoop->fired[j].fd];
            int mask = eventLoop->fired[j].mask;
            int fd = eventLoop->fired[j].fd;
            // if the read callback and write callback are same function
            // we should not call them twice, then a counter is needed
            int fired = 0; /* Number of events fired for current fd. */

            /* Normally we execute the readable event first, and the writable
             * event laster. This is useful as sometimes we may be able
             * to serve the reply of a query immediately after processing the
             * query.
             *
             * However if AE_BARRIER is set in the mask, our application is
             * asking us to do the reverse: never fire the writable event
             * after the readable. In such a case, we invert the calls.
             * This is useful when, for instance, we want to do things
             * in the beforeSleep() hook, like fsynching a file to disk,
             * before replying to a client. */
            int invert = fe->mask & AE_BARRIER;

            /* Note the "fe->mask & mask & ..." code: maybe an already
             * processed event removed an element that fired and we still
             * didn't processed, so we check if the event is still valid.
             *
             * Fire the readable event if the call sequence is not
             * inverted. */
            if (!invert && fe->mask & mask & AE_READABLE) {
                fe->rfileProc(eventLoop,fd,fe->clientData,mask);
                fired++;
            }

            /* Fire the writable event. */
            if (fe->mask & mask & AE_WRITABLE) {
                if (!fired || fe->wfileProc != fe->rfileProc) {
                    fe->wfileProc(eventLoop,fd,fe->clientData,mask);
                    fired++;
                }
            }

            /* If we have to invert the call, fire the readable event now
             * after the writable one. */
            if (invert && fe->mask & mask & AE_READABLE) {
                if (!fired || fe->wfileProc != fe->rfileProc) {
                    fe->rfileProc(eventLoop,fd,fe->clientData,mask);
                    fired++;
                }
            }

            processed++;
    ...

    if (flags & AE_TIME_EVENTS)
        processed += processTimeEvents(eventLoop);

    return processed; /* return the number of processed file/time events */
```

### aeMain
The `aeMain` is just a dead loop that executes the beforesleep hook and then polls and
 processes events. This is called in the end of main at `server.c`. If it is stopped,
 then the redis server will go down.

```c
void aeMain(aeEventLoop *eventLoop) {
    eventLoop->stop = 0;
    while (!eventLoop->stop) {
        if (eventLoop->beforesleep != NULL)
            eventLoop->beforesleep(eventLoop);
        aeProcessEvents(eventLoop, AE_ALL_EVENTS|AE_CALL_AFTER_SLEEP);
    }
}
```

### aeWait
There is also a `aeWait` function in this module. It is irrelevant to the eventloop.
But it also uses the `poll` event api. It uses it directly. It waits a fd until it is
 ready with a timeout. It treats `POLLERR` and `POLLHUP` as ready to write.

```c
int aeWait(int fd, int mask, long long milliseconds) {
    struct pollfd pfd;
    int retmask = 0, retval;

    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    if (mask & AE_READABLE) pfd.events |= POLLIN;
    if (mask & AE_WRITABLE) pfd.events |= POLLOUT;

    if ((retval = poll(&pfd, 1, milliseconds))== 1) {
        if (pfd.revents & POLLIN) retmask |= AE_READABLE;
        if (pfd.revents & POLLOUT) retmask |= AE_WRITABLE;
        if (pfd.revents & POLLERR) retmask |= AE_WRITABLE;
        if (pfd.revents & POLLHUP) retmask |= AE_WRITABLE;
        return retmask;
    } else {
        return retval;
    }
}
```

### References:
1. [github: redis ae.h](https://github.com/antirez/redis/blob/unstable/src/ae.h)
2. [github: redis ae.c](https://github.com/antirez/redis/blob/unstable/src/ae.c)
