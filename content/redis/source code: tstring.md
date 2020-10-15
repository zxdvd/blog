```metadata
tags: redis, sourecode
```

## redis source: get/set command implementation

The `t_string.c` implements the basic get/set related functions:

```
    GET key
    SET key value [EX seconds | PX milliseconds | KEEPTTL] [NX|XX]
    SETEX key seconds value
    PSETEX key milliseconds value
    SETNX key value
    GETSET key value

    MGET key1 key2 key3 ...
    MSET key1 value1 key2 value2 ...
    MSETNX key1 value1 key2 value2 ...

    INCR key
    INCRBY key int64_val
    INCRBYFLOAT key signed_float
    DECR key
    DECRBY key int64_val

    SETRANGE key offset value
    GETRANGE key start end

    STRLEN key
    APPEND key value
```

### set related
The `setGenericCommand` is the low level function that used by others. It first converts
 expire time to milliseconds. And then deals with `SET_NX` and `SET_XX` flags.

If it should set the key value, then it calls low level `genericSetKey` to set or update
 key value. Then server is marked as dirty and does some notifications and replies to client.

The `OBJ_SET_KEEPTTL` is very new flag that added in redis 6.0. Before this, `SET` on
 an existed key will remove its expire setting and make it a permanent key. If you only
 want to update its value, you need to get its TTL first and set new value with this TTL.
 I used to write a lua script to deal with this. With redis >= 6.0, just use the `KEEPTTL`
 flag.

```c
void setGenericCommand(client *c, int flags, robj *key, robj *val, robj *expire, int unit, robj *ok_reply, robj *abort_reply) {
    long long milliseconds = 0; /* initialized to avoid any harmness warning */

    if (expire) {
        if (getLongLongFromObjectOrReply(c, expire, &milliseconds, NULL) != C_OK)
            return;
        if (milliseconds <= 0) {
            addReplyErrorFormat(c,"invalid expire time in %s",c->cmd->name);
            return;
        }
        if (unit == UNIT_SECONDS) milliseconds *= 1000;
    }

    if ((flags & OBJ_SET_NX && lookupKeyWrite(c->db,key) != NULL) ||
        (flags & OBJ_SET_XX && lookupKeyWrite(c->db,key) == NULL))
    {
        addReply(c, abort_reply ? abort_reply : shared.null[c->resp]);
        return;
    }
    genericSetKey(c->db,key,val,flags & OBJ_SET_KEEPTTL);
    server.dirty++;
    if (expire) setExpire(c,c->db,key,mstime()+milliseconds);
    notifyKeyspaceEvent(NOTIFY_STRING,"set",key,c->db->id);
    if (expire) notifyKeyspaceEvent(NOTIFY_GENERIC,
        "expire",key,c->db->id);
    addReply(c, ok_reply ? ok_reply : shared.ok);
}
```

The `setCommand(client *c)` parses the command line arguments and then calls
 `setGenericCommand` to set key value.

Others like `setnxCommand`, `setexCommand`, `psetexCommand` are similar.

### get related
`GET` is simple, it calls `lookupKeyReadOrReply` and deals with errors.

```c
int getGenericCommand(client *c) {
    robj *o;

    if ((o = lookupKeyReadOrReply(c,c->argv[1],shared.null[c->resp])) == NULL)
        return C_OK;

    if (o->type != OBJ_STRING) {
        addReply(c,shared.wrongtypeerr);
        return C_ERR;
    } else {
        addReplyBulk(c,o);
        return C_OK;
    }
}
```

### getset
`GETSET` just tries to get first and then set if failed to get. It is atomic because
 there is only one thread that deals with all kinds of user commands. There is no
 concurrently updating or creating or expiry.

void getsetCommand(client *c) {
    if (getGenericCommand(c) == C_ERR) return;
    c->argv[2] = tryObjectEncoding(c->argv[2]);
    setKey(c->db,c->argv[1],c->argv[2]);
    notifyKeyspaceEvent(NOTIFY_STRING,"set",c->argv[1],c->db->id);
    server.dirty++;
}

### getrange and setrange
`GETRANGE` can get part value of a key while `SETRANGE` is used to update from an offset.

### mget and mset
`MGET` simply gets values in same order of keys passed in. It returns NULL for not existed
 key or key which is not a string.

`MSET` sets key value pairs one by one. `MSETNX` is atomic. It will either set all as
 `MSET` or do nothing if any key is existed.

### INCR, INCRBY and DECR
`INCR` will increase value of the key by one. It will set it to 1 via the key is not
 existed. You can control the increment via `INCRBY`. `DECR` and `DECRBY` is opposed
 sides.

`INCRBYFLOAT` is similar to `INCRBY`. It supports float number. There is one important
 difference between them. `INCRBY` creates a same `INCRBY` instruction in AOF and
  replication. But due to float precision problem, `INCRBYFLOAT` is replaced with
 `SET key value KEEPTTL` so that each replicas will get identical value.

```c
    /* Always replicate INCRBYFLOAT as a SET command with the final value
     * in order to make sure that differences in float precision or formatting
     * will not create differences in replicas or after an AOF restart. */
    aux1 = createStringObject("SET",3);
    rewriteClientCommandArgument(c,0,aux1);
    decrRefCount(aux1);
    rewriteClientCommandArgument(c,2,new);
    aux2 = createStringObject("KEEPTTL",7);
    rewriteClientCommandArgument(c,3,aux2);
    decrRefCount(aux2);
```

### references:
1. [github: redis tstring](https://github.com/antirez/redis/blob/unstable/src/t_string.c)
