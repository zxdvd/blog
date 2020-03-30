<!---
tags: redis, command, expire
-->

## redis command: expire
Following are expire related commands:

```
EXPIRE key seconds
PEXPIRE key milliseconds

EXPIREAT key timestamp
PEXPIREAT key milliseconds-timestamp

TTL key
PTTL key

PERSIST key
TOUCH key1 key2 key3 ...
```

EXPIRE sets N seconds expiration for a key from now. While `PEXPIRE` is almost same
 but the unit is millisecond.

The `EXPIREAT` sets a absolutely timestamp that a key will be expired. The unit for
 timestamp is second. For `PEXPIREAT`, the unit is millisecond.

A negative value for EXPIRE or timestamp before now for EXPIREAT will convert to
 delete the key.

TTL returns remaining time to live for a key, unit is second. For PTTL, the unit is
 millisecond. if server returns -2, it means the key is not existed. And -1 means
 no expire was set for the key.

### expire internal
Redis can have multiple separate db instances. Each db has a `dict *dict` to store all
 key value pairs. And a `dict *expires` to store expire time of key if set.

So the epxire command is just set timestamp in this dict and the ttl command is getting
 it from the same dict.

### command implementation
All the four expire related commands calls the `expireGenericCommand` underground.
For EXPIRE and PEXPIRE, it will pass current time as a base timestamp.

```c
/* EXPIRE key seconds */
void expireCommand(client *c) {
    expireGenericCommand(c,mstime(),UNIT_SECONDS);
}

/* EXPIREAT key time */
void expireatCommand(client *c) {
    expireGenericCommand(c,0,UNIT_SECONDS);
}

/* PEXPIRE key milliseconds */
void pexpireCommand(client *c) {
    expireGenericCommand(c,mstime(),UNIT_MILLISECONDS);
}

/* PEXPIREAT key ms_time */
void pexpireatCommand(client *c) {
    expireGenericCommand(c,0,UNIT_MILLISECONDS);
}
```

For `expireGenericCommand`, second parameter is base timestamp, the third is unit.
If the final to-be-set timestamp is less than now, then convert it to delete.

```c
void expireGenericCommand(client *c, long long basetime, int unit) {
    robj *key = c->argv[1], *param = c->argv[2];
    long long when; /* unix time in milliseconds when the key will expire. */

    if (getLongLongFromObjectOrReply(c, param, &when, NULL) != C_OK)
        return;

    // convert second to ms and added basetime, then it becomes absolutely timestamp (ms)
    if (unit == UNIT_SECONDS) when *= 1000;
    when += basetime;

    /* No key, return zero. */
    if (lookupKeyWrite(c->db,key) == NULL) {
        addReply(c,shared.czero);
        return;
    }

    /* EXPIRE with negative TTL, or EXPIREAT with a timestamp into the past
     * should never be executed as a DEL when load the AOF or in the context
     * of a slave instance.
     *
     * Instead we take the other branch of the IF statement setting an expire
     * (possibly in the past) and wait for an explicit DEL from the master. */
    if (when <= mstime() && !server.loading && !server.masterhost) {
        robj *aux;

        int deleted = server.lazyfree_lazy_expire ? dbAsyncDelete(c->db,key) :
                                                    dbSyncDelete(c->db,key);
        serverAssertWithInfo(c,key,deleted);
        server.dirty++;

        /* Replicate/AOF this as an explicit DEL or UNLINK. */
        aux = server.lazyfree_lazy_expire ? shared.unlink : shared.del;
        rewriteClientCommandVector(c,2,aux,key);
        signalModifiedKey(c->db,key);
        notifyKeyspaceEvent(NOTIFY_GENERIC,"del",key,c->db->id);
        addReply(c, shared.cone);
        return;
    } else {
        // set expire in the dict and reply 1, then do related hooks
        setExpire(c,c->db,key,when);
        addReply(c,shared.cone);
        signalModifiedKey(c->db,key);
        notifyKeyspaceEvent(NOTIFY_GENERIC,"expire",key,c->db->id);
        server.dirty++;
        return;
    }
}
```

### PERSIST, TTL, PTTL
The PERSIST will remove the expire setting for a key if exists to make it persistent
 key.

The TTL and PTTL just get the ms_timestamp from the expire dict of db if exists, and
 then substract current timestamp from it and return to client.

### TOUCH key1 key2 key3
The `TOUCH` command just calls the `lookupKeyRead` that will update LFU values or LRU
 clock values for keys. This may avoid keys being deleted due to too old or not used.

### References:
1. [github: redis expire](https://github.com/antirez/redis/blob/unstable/src/expire.c)
