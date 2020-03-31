<!---
tags: redis, expire
-->

Each db uses a dict to store the expired timestamp for keys that have expire set.
But then how to detect and expire keys using the dict? Traverse the whole dict to check
 if it is expired or not?

Redis deals with expired keys in following ways:

#### check it each time you lookup a key
Each time you called `lookupKeyRead` or `lookupKeyWrite`, it will call `expireIfNeeded`.
And the `expireIfNeeded` will try to delete the key if it is expired.
So a key is checked each time it is accessed by the user.

#### server's cron job
The redis server will periodically run `activeExpireCycle` to deal with the expired
 keys. This function supports two mode, one is slow and another is fast. The slow
 one will run longer and clear more expired keys.

The slow one is called in the `databaseCron`. The fast one is called in the
 `beforeSleep` hooks that will be run each time before the eventloop polling.

Both of them are run at master only with `server.active_expire_enabled` being true.

Since this is run at redis main thread, it must last very long each time it runs.
Then the function `activeExpireCycle` has many static variables to save the running
 state so that it can continue from previous state on next running.

```c
    /* This function has some global state in order to continue the work
     * incrementally across calls. */
    static unsigned int current_db = 0; /* Last DB tested. */
    static int timelimit_exit = 0;      /* Time limit hit in previous call? */
    static long long last_fast_cycle = 0; /* When last fast cycle ran. */
```

It will deal with each db one by one. So it needs to record current db. This is the
 most important one compared to `timelimit_exit` and `last_fast_cycle` that are used
 for optimizing.

For each db, it has a `unsigned long expires_cursor;` field to record the current
 checked stot since a db can be very large and it cannot be fully checked during
 one cycle.

```c
// server.h (only part of fields is copied here)
typedef struct redisDb {
    dict *dict;                 /* The keyspace for this DB */
    dict *expires;              /* Timeout of keys with a timeout set */
    int id;                     /* Database ID */
    unsigned long expires_cursor; /* Cursor of the active expire cycle. */
} redisDb;
```

The `timelimit_exit` is used to mark whether the previous cycle is existed due to
 reaching time limit or not.

This function will exit if time limit reached to avoid block main thread too much time.
However, if all dbs are checked and there is still time left. The next fast cycle will
 be skipped.

```c
    if (type == ACTIVE_EXPIRE_CYCLE_FAST) {
        if (!timelimit_exit &&
            server.stat_expired_stale_perc < config_cycle_acceptable_stale)
            return;
        last_fast_cycle = start;
    }
```

Redis has control about minimum gap between two fast mode expireCycle. Then the
 `last_fast_cycle` here is used to log last fast cycle running time.

```c
    if (type == ACTIVE_EXPIRE_CYCLE_FAST) {
        if (start < last_fast_cycle + (long long)config_cycle_fast_duration*2)
            return;
        last_fast_cycle = start;
    }
```

The key difference between fast mode and slow mode is that how long this function should
 run. They set different `timelimit`:

```c
    timelimit = config_cycle_slow_time_perc*1000000/server.hz/100;
    timelimit_exit = 0;
    if (timelimit <= 0) timelimit = 1;

    if (type == ACTIVE_EXPIRE_CYCLE_FAST)
        timelimit = config_cycle_fast_duration; /* in microseconds. */
```

With default config, fast mode will run 1ms and slow mode will run 25 ms.

Following is the core code of this function:

```c
    // loop will stop if no time left (timelimit_exit == 0)
    for (j = 0; j < dbs_per_call && timelimit_exit == 0; j++) {
        /* Expired and checked in a single loop. */
        unsigned long expired, sampled;

        redisDb *db = server.db+(current_db % server.dbnum);
        current_db++;

        do {
            iteration++;

            /* If there is nothing to expire try next DB ASAP. */
            if ((num = dictSize(db->expires)) == 0) {
                db->avg_ttl = 0;
                break;
            }
            slots = dictSlots(db->expires);
            now = mstime();

            // no need to check if the fill ratio of slots is less than 1%
            if (num && slots > DICT_HT_INITIAL_SIZE &&
                (num*100/slots < 1)) break;

            /* The main collection cycle. Sample random keys among keys
             * with an expire set, checking for expired ones. */
            expired = 0;
            sampled = 0;
            ttl_sum = 0;
            ttl_samples = 0;

            // at most scan 20 keys each loop, at most 400 slots
            if (num > config_keys_per_loop)
                num = config_keys_per_loop;

            long max_buckets = num*20;
            long checked_buckets = 0;

            while (sampled < num && checked_buckets < max_buckets) {
                for (int table = 0; table < 2; table++) {
                    if (table == 1 && !dictIsRehashing(db->expires)) break;

                    unsigned long idx = db->expires_cursor;
                    idx &= db->expires->ht[table].sizemask;
                    dictEntry *de = db->expires->ht[table].table[idx];
                    long long ttl;

                    /* Scan the current bucket of the current table. */
                    checked_buckets++;
                    while(de) {
                        /* Get the next entry now since this entry may get
                         * deleted. */
                        dictEntry *e = de;
                        de = de->next;

                        ttl = dictGetSignedIntegerVal(e)-now;
                        if (activeExpireCycleTryExpire(db,e,now)) expired++;
                        if (ttl > 0) {
                            /* We want the average TTL of keys yet
                             * not expired. */
                            ttl_sum += ttl;
                            ttl_samples++;
                        }
                        sampled++;
                    }
                }
                db->expires_cursor++;
            }
            total_expired += expired;
            total_sampled += sampled;

            /* Update the average TTL stats for this database. */
            if (ttl_samples) {
                long long avg_ttl = ttl_sum/ttl_samples;

                /* Do a simple running average with a few samples.
                 * We just use the current estimate with a weight of 2%
                 * and the previous estimate with a weight of 98%. */
                if (db->avg_ttl == 0) db->avg_ttl = avg_ttl;
                db->avg_ttl = (db->avg_ttl/50)*49 + (avg_ttl/50);
            }

            /* We can't block forever here even if there are many keys to
             * expire. So after a given amount of milliseconds return to the
             * caller waiting for the other active expire cycle. */
            if ((iteration & 0xf) == 0) { /* check once every 16 iterations. */
                elapsed = ustime()-start;
                if (elapsed > timelimit) {
                    timelimit_exit = 1;
                    server.stat_expired_time_cap_reached_count++;
                    break;
                }
            }
            /* We don't repeat the cycle for the current database if there are
             * an acceptable amount of stale keys (logically expired but yet
             * not reclaimed). */
        } while (sampled == 0 ||
                 (expired*100/sampled) > config_cycle_acceptable_stale);
    }
```

#### the evict policies
The evict policies may also delete expired keys and I'll show this in the post about evict.

### References:
1. [github: redis expire](https://github.com/antirez/redis/blob/unstable/src/expire.c)
