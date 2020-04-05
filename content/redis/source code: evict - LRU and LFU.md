<!---
tags: redis, source code, evict
-->

We know that redis has many strategies to evict keys when reaching max memory. The
 four evict methods are: RANDOM, LRU, LFU, TTL.

A key point here: any of the the LRU, LFU, TTL is not exact implementation. It's
 an approximate method that try best to get similar result.

This is trade off of performance. Exact LRU, LFU or TTL will use more memory since
 you need to record exact order in sorted list.

### RANDOM evict

The RANDOM method is easy. Just fetch keys from dict randomly.

```c
// evict.c freeMemoryIfNeeded
        /* volatile-random and allkeys-random policy */
        else if (server.maxmemory_policy == MAXMEMORY_ALLKEYS_RANDOM ||
                 server.maxmemory_policy == MAXMEMORY_VOLATILE_RANDOM)
        {
            // the next_db is a static variable
            for (i = 0; i < server.dbnum; i++) {
                j = (++next_db) % server.dbnum;
                db = server.db+j;
                dict = (server.maxmemory_policy == MAXMEMORY_ALLKEYS_RANDOM) ?
                        db->dict : db->expires;
                if (dictSize(dict) != 0) {
                    de = dictGetRandomKey(dict);
                    bestkey = dictGetKey(de);
                    bestdbid = j;
                    break;
                }
            }
        }
```

For LRU, LFU, TTL, it computes idle time of each sampled keys. The max idle time key
 will be inserted into the pool and may be deleted later.

### TTL evict

For TTL method, the idle time is got by max long int minus the expire time. So keys
 that will expire most recently will get max idle time.

```c
            idle = ULLONG_MAX - (long)dictGetVal(de);
```

### LRU and LFU
For LRU and LFU, redis uses a extra field in `robj` to record related info. It uses
 24 bits `lru`. Though the name is `lru`, the LFU method also uses it. Maybe it's
 better to use an union data structure here.

This field is updated each time the key is accessed.

```c
// db.c
robj *lookupKey(redisDb *db, robj *key, int flags) {
    dictEntry *de = dictFind(db->dict,key->ptr);
    if (de) {
        robj *val = dictGetVal(de);

        /* Update the access time for the ageing algorithm.
         * Don't do it if we have a saving child, as this will trigger
         * a copy on write madness. */
        if (!hasActiveChildProcess() && !(flags & LOOKUP_NOTOUCH)){
            if (server.maxmemory_policy & MAXMEMORY_FLAG_LFU) {
                updateLFU(val);
            } else {
                val->lru = LRU_CLOCK();
            }
        }
        return val;
    } else {
        return NULL;
    }
}
```

### LRU
When using LRU method, it will use the `lru` field of the `robj` to log current clock
 time. Since it only 24 bits, it converts millisecond to second and only the lower 24
 bits are used.

Since this function is called so frequently, redis cache it at the `server.lruclock` and
 will refresh it in the `serverCron` so that you needn't to call it so frequently.

```c
 unsigned int getLRUClock(void) {
    return (mstime()/LRU_CLOCK_RESOLUTION) & LRU_CLOCK_MAX;
}

/* This function is used to obtain the current LRU clock.
 * If the current resolution is lower than the frequency we refresh the
 * LRU clock (as it should be in production servers) we return the
 * precomputed value, otherwise we need to resort to a system call. */
unsigned int LRU_CLOCK(void) {
    unsigned int lruclock;
    if (1000/server.hz <= LRU_CLOCK_RESOLUTION) {
        lruclock = server.lruclock;
    } else {
        lruclock = getLRUClock();
    }
    return lruclock;
}
```

The `unsigned long long estimateObjectIdleTime(robj *o)` is used to calculate the idle
 time of the LRU method, it just substracts current time by the recorded `o.lru`. It
 the result is negative, then the clock may overflow 24 bits and a `LRU_CLOCK_MAX` will
 be added.

### LFU
If you choose the LFU method, the 24 bits `lru` field of `robj` will be split into two
 parts, the higher 16 bits is used to record the timestamp (minute unit) and the lower
 8 bits is used as a logarithm counter.

So what is logarithm counter? The counter grows slower and slower as it becomes larger.
For example, if the counter is 10, it may be increased by 1 if accessed for 50 times.
If the counter becomes 20, it needs to be accessed by 200 times to add 1. If it is 100,
 it needs 1000 accessing.

If the counter is small (less than 5), it will be added faster so that new keys won't
 be evicted quickly.

There is a config `lfu_log_factor` that can be used to control how slow the increment
 could be. It is 10 by default. You can change this to 100, so that it needs 500 accessing
 to become 11 from 10.

```c
uint8_t LFULogIncr(uint8_t counter) {
    if (counter == 255) return 255;
    double r = (double)rand()/RAND_MAX;
    double baseval = counter - LFU_INIT_VAL;
    if (baseval < 0) baseval = 0;
    double p = 1.0/(baseval*server.lfu_log_factor+1);
    if (r < p) counter++;
    return counter;
}
```

The `LFU` here is not simple LFU. Access time is also considered. If a key is not accessed
 recently, the counter may be decreased. A yesterday frequent key may not be frequent key
 of today.

```c
unsigned long LFUDecrAndReturn(robj *o) {
    unsigned long ldt = o->lru >> 8;
    unsigned long counter = o->lru & 255;
    unsigned long num_periods = server.lfu_decay_time ? LFUTimeElapsed(ldt) / server.lfu_decay_time : 0;
    if (num_periods)
        counter = (num_periods > counter) ? 0 : counter - num_periods;
    return counter;
}
```

For example, if the counter is 50, and the last access time of the key is 5 minutes
 ago, then counter will be substracted by 5 and become 45.

There is also a config `lfu_decay_time` here that you can use it to control the decreasing
 speed. For above code we can find that it also deal with small counter specially.

The `updateLFU` function in `db.c` is called in `lookupKey` then the LFU of key is updated
 each time it is accessed if LFU method is configed.

```c
void updateLFU(robj *val) {
    unsigned long counter = LFUDecrAndReturn(val);
    counter = LFULogIncr(counter);
    val->lru = (LFUGetTimeInMinutes()<<8) | counter;
}
```

For the LFU method, the idle time is very easy to calculated, just 255 minus the counter.

### evict to pool
The `evictionPoolPopulate` is the function that will fetch some keys from dict and insert
 them into the eviction pool.

It uses the `dictGetSomeKeys` to fetch keys, the number of keys can be configed by option
 `maxmemory_samples` which is 5 by default.

The `dictGetSomeKeys` will start from a random point (i = random() & maxsizemask).

The `freeMemoryIfNeeded` will called if maxmemory is reached. And it will try to find
 best key to delete according the evict strategy.

### References:
- [official doc: redis lru cache](https://redis.io/topics/lru-cache)
2. [github: redis evict.c](https://github.com/antirez/redis/blob/unstable/src/evict.c)
