<!---
tags: redis, aof
-->

## redis aof: command log, buffer and fsync

Redis supports two persistence methods: RDB and AOF. The RDB is a snapshot of all data
 in memory at a specific time. And the AOF is sequence log of each write to the database.

Using AOF, you can recover redis to any time of point. But it is very slow compared with
 RDB since it needs to replay all history log records one by one.

I won't write about pros and cons of them at this post. I'll analyze how AOF is recorded.

### propagation
In article [how redis process command](./how redis process a command.md) we can find that
 the `processCommand` will call `call(client *c, int flags)` to execute the query command.

After the `c->cmd->proc(c);`, it will deal with propagation:

```c
// server.c: call
    /* Propagate the command into the AOF and replication link */
    if (flags & CMD_CALL_PROPAGATE &&
        (c->flags & CLIENT_PREVENT_PROP) != CLIENT_PREVENT_PROP)
    {
        ....
        /* Call propagate() only if at least one of AOF / replication
         * propagation is needed. Note that modules commands handle replication
         * in an explicit way, so we never replicate them automatically. */
        if (propagate_flags != PROPAGATE_NONE && !(c->cmd->flags & CMD_MODULE))
            propagate(c->cmd,c->db->id,c->argv,c->argc,propagate_flags);
    }
```

The function `propagate` will handle AOF or replication if needed, e.g., database changed.
It will try to log the change if AOF is on and `PROPAGATE_AOF` flag is enabled.

```c
// server.c
void propagate(struct redisCommand *cmd, int dbid, robj **argv, int argc,
               int flags)
{
    if (server.aof_state != AOF_OFF && flags & PROPAGATE_AOF)
        feedAppendOnlyFile(cmd,dbid,argv,argc);
    if (flags & PROPAGATE_REPL)
        replicationFeedSlaves(server.slaves,dbid,argv,argc);
}
```

### write AOF record
The `feedAppendOnlyFile` function in `aof.c` tries to convert the command to a RESP
 buffer and append to `server.aof_buf`.

Firstly, it will check if the dictid is same as last AOF record. If not, it will
 generate a `SELECT DBID` command and append it into the buffer.

Secondly, some commands need to do some special transformation. For example, all of
 `expire` , `pexpire`, `expireat` will be converted to `pexpireat` since it is absolute
 time and unit is millisecond.

And the `setex` and `psetex` will be converted to two commands: `set` and `pexpireat`.
All other commands are appended into buffer via `catAppendOnlyGenericCommand`.

The `catAppendOnlyGenericCommand` is simply concat the command and arguments using the
 RESP protocol.

Finally, this buffer is appended into the `server.aof_buf`. The redis server will flush
 `server.aof_buf` to disk periodically according the config.

```c
void feedAppendOnlyFile(struct redisCommand *cmd, int dictid, robj **argv, int argc) {
    sds buf = sdsempty();

    if (dictid != server.aof_selected_db) {
        char seldb[64];

        snprintf(seldb,sizeof(seldb),"%d",dictid);
        buf = sdscatprintf(buf,"*2\r\n$6\r\nSELECT\r\n$%lu\r\n%s\r\n",
            (unsigned long)strlen(seldb),seldb);
        server.aof_selected_db = dictid;
    }

    if (cmd->proc == expireCommand || cmd->proc == pexpireCommand ||
        cmd->proc == expireatCommand) {
        /* Translate EXPIRE/PEXPIRE/EXPIREAT into PEXPIREAT */
        buf = catAppendOnlyExpireAtCommand(buf,cmd,argv[1],argv[2]);
    } else if (cmd->proc == setexCommand || cmd->proc == psetexCommand) {
        /* Translate SETEX/PSETEX to SET and PEXPIREAT */
        tmpargv[0] = createStringObject("SET",3);
        tmpargv[1] = argv[1];
        tmpargv[2] = argv[3];
        buf = catAppendOnlyGenericCommand(buf,3,tmpargv);
        decrRefCount(tmpargv[0]);
        buf = catAppendOnlyExpireAtCommand(buf,cmd,argv[1],argv[2])
    } else if (cmd->proc == setCommand && argc > 3) {
        ...
    } else {
        buf = catAppendOnlyGenericCommand(buf,argc,argv);
    }

    if (server.aof_state == AOF_ON)
        server.aof_buf = sdscatlen(server.aof_buf,buf,sdslen(buf));

    /* If a background append only file rewriting is in progress we want to
     * accumulate the differences between the child DB and the current one
     * in a buffer, so that when the child process will do its work we
     * can append the differences to the new append only file. */
    if (server.aof_child_pid != -1)
        aofRewriteBufferAppend((unsigned char*)buf,sdslen(buf));

    sdsfree(buf);
}

### flush AOF buffer to file and fsync
Redis needs to write temporary data buffered in `server.aof_buf` to file in time to
 avoid it uses too much memory and loses too much data in case of server crashing.

The `flushAppendOnlyFile` function handles the flush and fsync. It is called at
`beforeSleep`. It will also be called in `serverCron` if there is postponed fsync
 or previous fsync is failed.

It is also called in graceful shutdown to avoid losing data.

```c
void flushAppendOnlyFile(int force) {
    ...
    nwritten = aofWrite(server.aof_fd,server.aof_buf,sdslen(server.aof_buf));
    ...
    if (server.aof_fsync == AOF_FSYNC_ALWAYS) {
        redis_fsync(server.aof_fd); /* Let's try to get this data on the disk */
    } else if ((server.aof_fsync == AOF_FSYNC_EVERYSEC &&
                server.unixtime > server.aof_last_fsync)) {
        if (!sync_in_progress) {
            aof_background_fsync(server.aof_fd);
            server.aof_fsync_offset = server.aof_current_size;
        }
    }
}
```

Above is the core code of the function `flushAppendOnlyFile`:

- write buffer to file
- do fsync directly (this will block) or asynchronously (non-blocking)

It seems very simple but the code is complex due to many corner cases.

- It needs to deal with short write. It records the file offset before written and
 will truncate the file the origin length if short write happens.

- need to choose to do sync or async fsync accordingly

- It the fsync is set to `AOF_FSYNC_ALWAYS`, it will exit if got error

