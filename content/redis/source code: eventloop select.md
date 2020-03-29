<!---
tags: redis, eventloop, select
-->

The `select.c` provides the low level apis needed by eventloop using `select`. It will
 be used when port, epoll, kqueue are not existed. Let's look at select api first.

```c
int select(int nfds, fd_set *readfds, fd_set *writefds,
                     fd_set *exceptfds, struct timeval *timeout);
void FD_CLR(int fd, fd_set *set);
int  FD_ISSET(int fd, fd_set *set);
void FD_SET(int fd, fd_set *set);
void FD_ZERO(fd_set *set);
```

The `fd_set` is fixed buffer. You can pass any of read/write/error fd_set to `select`.
The `nfds` is maxfs + 1. The above four macros are used to control the fd_set buffer.

Key point, `select` will modify the fd_set passed in, it returns ready fds using same
 pointers. So if you need to select again and again, you need to copy fd_sets before
 passing to select.

All apis in `ae_select.c` and `ae_epoll.c` are static since then should not be exposed to
 outside. You should use high level apis in `ae.c`.

### aeApiState defined in select
The `aeApiState` defines read fd_set and write fd_set and their backups.
No need to define the maxfd since the eventloop has it.

```c
typedef struct aeApiState {
    fd_set rfds, wfds;
    /* We need to have a copy of the fd sets as it's not safe to reuse
     * FD sets after select(). */
    fd_set _rfds, _wfds;
} aeApiState;
```

### aeApiCreate
The high level api `aeCreateEventLoop` will call this when compiler chose `select`.
This function will initialize the `aeApiState` and assign it to eventloop's apidata.

```c
static int aeApiCreate(aeEventLoop *eventLoop) {
    aeApiState *state = zmalloc(sizeof(aeApiState));

    if (!state) return -1;
    FD_ZERO(&state->rfds);
    FD_ZERO(&state->wfds);
    eventLoop->apidata = state;
    return 0;
}
```

### add/delete event
Add event fd to eventloop using FD_SET macro.
Delete is almost same, just use FD_CLR.

```c
static int aeApiAddEvent(aeEventLoop *eventLoop, int fd, int mask) {
    aeApiState *state = eventLoop->apidata;

    if (mask & AE_READABLE) FD_SET(fd,&state->rfds);
    if (mask & AE_WRITABLE) FD_SET(fd,&state->wfds);
    return 0;
}
```

### polling
As explained above, `select` will change data you passed in. Then each time before
 calling `select`, need to use `memcpy` to copy the fd_sets.

After got some ready events, just put them into evenloop's fired array.

```c
static int aeApiPoll(aeEventLoop *eventLoop, struct timeval *tvp) {
    aeApiState *state = eventLoop->apidata;
    int retval, j, numevents = 0;

    memcpy(&state->_rfds,&state->rfds,sizeof(fd_set));
    memcpy(&state->_wfds,&state->wfds,sizeof(fd_set));

    retval = select(eventLoop->maxfd+1,
                &state->_rfds,&state->_wfds,NULL,tvp);
    if (retval > 0) {
        for (j = 0; j <= eventLoop->maxfd; j++) {
            int mask = 0;
            aeFileEvent *fe = &eventLoop->events[j];

            if (fe->mask == AE_NONE) continue;
            if (fe->mask & AE_READABLE && FD_ISSET(j,&state->_rfds))
                mask |= AE_READABLE;
            if (fe->mask & AE_WRITABLE && FD_ISSET(j,&state->_wfds))
                mask |= AE_WRITABLE;
            eventLoop->fired[numevents].fd = j;
            eventLoop->fired[numevents].mask = mask;
            numevents++;
        }
    }
    return numevents;
}
```

### References:
- [github: redis ae_select.c](https://github.com/antirez/redis/blob/unstable/src/ae_select.c)
