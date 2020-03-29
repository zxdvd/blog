<!---
tags: redis, eventloop, epoll
-->

Both `epoll` and `select` are multiplexing event handling system supported on linux.
`epoll` is always preferred if portable is not considered:

- select's fd_set is fixed buffer, it only supports 1024 fds at most by default
- select needs to copy fd_sets since it will be updated and reused as returned fd_sets

The advantage of select is that it is supported by many OS while epoll is linux only.

Redis will of course use epoll on linux.

### event state
For epoll, a epfd is needed to store the epoll instance. And epoll_event to store all
 events.

```c
typedef struct aeApiState {
    int epfd;
    struct epoll_event *events;
} aeApiState;
```

### aeApiCreate
For `select`, no need to malloc for fd_set since it is fixed buffer. But for epoll,
 we need a `struct epoll_event` for each event.

Then in `aeApiCreate`, it allocates memory for events and create a epoll instance.

```c
static int aeApiCreate(aeEventLoop *eventLoop) {
    aeApiState *state = zmalloc(sizeof(aeApiState));

    if (!state) return -1;
    state->events = zmalloc(sizeof(struct epoll_event)*eventLoop->setsize);
    if (!state->events) {
        zfree(state);
        return -1;
    }
    state->epfd = epoll_create(1024); /* 1024 is just a hint for the kernel */
    if (state->epfd == -1) {
        zfree(state->events);
        zfree(state);
        return -1;
    }
    eventLoop->apidata = state;
    return 0;
}
```

### aeApiResize
This function only reallocs the events in aeApiState.

### add/delete event
For add or delete, just initialize a epoll_event data, and then use epoll_ctl to send
 commands (EPOLL_CTL_ADD, EPOLL_CTL_MOD, EPOLL_CTL_DEL) to the epoll instance.

```c
static int aeApiAddEvent(aeEventLoop *eventLoop, int fd, int mask) {
    aeApiState *state = eventLoop->apidata;
    struct epoll_event ee = {0}; /* avoid valgrind warning */
    /* If the fd was already monitored for some event, we need a MOD
     * operation. Otherwise we need an ADD operation. */
    int op = eventLoop->events[fd].mask == AE_NONE ?
            EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    ee.events = 0;
    mask |= eventLoop->events[fd].mask; /* Merge old events */
    if (mask & AE_READABLE) ee.events |= EPOLLIN;
    if (mask & AE_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.fd = fd;
    if (epoll_ctl(state->epfd,op,fd,&ee) == -1) return -1;
    return 0;
}
```

Deleting is same as adding, it may use `EPOLL_CTL_MOD` or `EPOLL_CTL_DEL` instead.

### polling
The `aeApiPoll` polls ready events from epoll instance and then set the them in the
 `fired` field of eventloop.

Set `timeout` to -1 means blocking forever until a fd is ready or thread is interrupted
 by a signal.

```c
static int aeApiPoll(aeEventLoop *eventLoop, struct timeval *tvp) {
    aeApiState *state = eventLoop->apidata;
    int retval, numevents = 0;

    retval = epoll_wait(state->epfd,state->events,eventLoop->setsize,
            tvp ? (tvp->tv_sec*1000 + tvp->tv_usec/1000) : -1);
    if (retval > 0) {
        int j;

        numevents = retval;
        for (j = 0; j < numevents; j++) {
            int mask = 0;
            struct epoll_event *e = state->events+j;

            if (e->events & EPOLLIN) mask |= AE_READABLE;
            if (e->events & EPOLLOUT) mask |= AE_WRITABLE;
            if (e->events & EPOLLERR) mask |= AE_WRITABLE|AE_READABLE;
            if (e->events & EPOLLHUP) mask |= AE_WRITABLE|AE_READABLE;
            eventLoop->fired[j].fd = e->data.fd;
            eventLoop->fired[j].mask = mask;
        }
    }
    return numevents;
}
```

### References:
- [github: redis ae_epoll.c](https://github.com/antirez/redis/blob/unstable/src/ae_epoll.c)
