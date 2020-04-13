<!---
tags: redis, aof
-->

## redis: aof rewrite
We know that AOF is incremental log of updates. The aof file will become too large
 quickly. And too much records are useless since keys update and expire frequently.
It will waste too much space and take too much time when loading AOF file.

The `AOF rewrite` is used to deal with this situation. The server will trigger rewrite
 if the aof file grows enough large. And you can issue a command to trigger a rewrite
 directly.

Redis will fork a child process to do the rewrite. With the `fork`, the child process
 can get s pure static snapshot of the redis server. And it won't block the server to
 continue serve clients. The bad side is that it may use much memory due to COW.


### rewrite related settings
Following is aof rewrite related fields defined in `struct redisServer`.

```c
    int aof_rewrite_perc;           /* Rewrite AOF if % growth is > M and... */
    off_t aof_rewrite_min_size;     /* the AOF file is at least N bytes. */
    off_t aof_rewrite_base_size;    /* AOF size on latest startup or rewrite. */
    int aof_rewrite_scheduled;      /* Rewrite once BGSAVE terminates. */
    pid_t aof_child_pid;            /* PID if rewriting process */
    time_t aof_rewrite_time_last;   /* Time used by last AOF rewrite run. */
    time_t aof_rewrite_time_start;  /* Current AOF rewrite start time. */
    int aof_lastbgrewrite_status;   /* C_OK or C_ERR */
    int aof_rewrite_incremental_fsync;/* fsync incrementally while aof rewriting? */
    int aof_use_rdb_preamble;       /* Use RDB preamble on AOF rewrites. */

    /* AOF pipes used to communicate between parent and child during rewrite. */
    int aof_pipe_write_data_to_child;
    int aof_pipe_read_data_from_parent;
    int aof_pipe_write_ack_to_parent;
    int aof_pipe_read_ack_from_child;
    int aof_pipe_write_ack_to_child;
    int aof_pipe_read_ack_from_parent;
    int aof_stop_sending_diff;     /* If true stop sending accumulated diffs
                                      to child process. */
    sds aof_child_diff;             /* AOF diff accumulator child side. */
```

The `aof_rewrite_perc` is a percentage threshold of increased size of aof file that can
 trigger a rewrite. By default it is 100 which means it will trigger a rewrite if the
 aof file increased by 100%. You can config it larger if you want the rewrite happened
 less frequently.

The `aof_rewrite_min_size` is a minimum size to trigger a rewrite as no need to do rewrite
 when the aof file is very small. By default this is set to 64MiB.

The `aof_rewrite_base_size` is the base size to compute the growth percentage. It is 0
 when the server is initialized and then becomes size of the loaded aof file. It will be
 updated after each rewrite.

The `aof_rewrite_scheduled` is a mark to let the server do rewrite later. If there is any
 other process (aof, rdb, module) that is still running, rewrite will be scheduled to run
  later. I think this is aimed to reduce memory and IO overhead.

The `aof_child_pid` is the pid of aof child process.

The `aof_rewrite_time_start` is start time of aof while `aof_rewrite_time_last` is duration.

The `aof_rewrite_incremental_fsync` is enabled by default. Then the rewrite file will be
 fsynced when fixed bytes has been written (32MiB by default).

The `aof_use_rdb_preamble` is enabled by default so that redis will do a `rdb save` instead
 of traditional aof rewrite.

The `aof_pipe_*` are used to communicate between parent and child to send and acknowledge
 incremental aof records.

The `aof_child_diff` is a sds string to receive aof records during the fork. It will also
 be written into the rewrite file after whole db is saved.

### how rewrite starts
The rewrite will be triggered periodically to reduce aof file size. Following is related
 code. It needs to check if the size is large enough. `rewriteAppendOnlyFileBackground` is
 the worker to do the rewrite.

```c
// server.c: serverCron
        /* Trigger an AOF rewrite if needed. */
        if (server.aof_state == AOF_ON &&
            !hasActiveChildProcess() &&
            server.aof_rewrite_perc &&
            server.aof_current_size > server.aof_rewrite_min_size)
        {
            long long base = server.aof_rewrite_base_size ?
                server.aof_rewrite_base_size : 1;
            long long growth = (server.aof_current_size*100/base) - 100;
            if (growth >= server.aof_rewrite_perc) {
                serverLog(LL_NOTICE,"Starting automatic rewriting of AOF on %lld%% growth",growth);
                rewriteAppendOnlyFileBackground();
            }
        }
```

It can also be triggered by command `BGREWRITEAOF`. Client will get error if there is
 a rewrite process running.

Since you can enable the AOF on the fly. It will also be triggered if you change
 `appendonly` config from `no` to `yes`.

### rewrite process
The `rewriteAppendOnlyFileBackground` will setup pipes and then fork. The forked child
 process will call `rewriteAppendOnlyFile(tmpfile)` to do the rewrite.

The `rewriteAppendOnlyFile(tmpfile)` will open the file and initialize as a `rio` object.
Then it will do the rewrite using either `RDB save` or `AOF append`. The rdb is binary
 and the aof is text RESP protocol.

```c
int rewriteAppendOnlyFile(char *filename) {
    ...
    if (server.aof_use_rdb_preamble) {
        int error;
        if (rdbSaveRio(&aof,&error,RDBFLAGS_AOF_PREAMBLE,NULL) == C_ERR) {
            errno = error;
            goto werr;
        }
    } else {
        if (rewriteAppendOnlyFileRio(&aof) == C_ERR) goto werr;
    }
    if (fflush(fp) == EOF) goto werr;
    if (fsync(fileno(fp)) == -1) goto werr;
    ...
}
```

Then this function tries to receive incremental aof records from parent and save.
After this, it will flush and fsync. If everything is ok, it will rename this file
 to origin name. Otherwise, this temporary file is closed and unlinked. The origin
 file won't be touched.

The `rewriteAppendOnlyFileRio` function will loop each database and then iterate all
 keys of each database to convert key value pairs to AOF records and append to the
 rio object.

```c
int rewriteAppendOnlyFileRio(rio *aof) {
    for (j = 0; j < server.dbnum; j++) {
        ...
        /* SELECT the new DB */
        if (rioWrite(aof,selectcmd,sizeof(selectcmd)-1) == 0) goto werr;
        if (rioWriteBulkLongLong(aof,j) == 0) goto werr;

        /* Iterate this DB writing every entry */
        while((de = dictNext(di)) != NULL) {
            ...
            keystr = dictGetKey(de);
            o = dictGetVal(de);
            initStaticStringObject(key,keystr);

            expiretime = getExpire(db,&key);

            /* Save the key and associated value */
            if (o->type == OBJ_STRING) {
                /* Emit a SET command */
                char cmd[]="*3\r\n$3\r\nSET\r\n";
                if (rioWrite(aof,cmd,sizeof(cmd)-1) == 0) goto werr;
                /* Key and value */
                if (rioWriteBulkObject(aof,&key) == 0) goto werr;
                if (rioWriteBulkObject(aof,o) == 0) goto werr;
            } else if (o->type == OBJ_LIST) {
                if (rewriteListObject(aof,&key,o) == 0) goto werr;
            } else if (o->type == OBJ_SET) {
                if (rewriteSetObject(aof,&key,o) == 0) goto werr;
            } else if (o->type == OBJ_ZSET) {
                if (rewriteSortedSetObject(aof,&key,o) == 0) goto werr;
            } else if (o->type == OBJ_HASH) {
                if (rewriteHashObject(aof,&key,o) == 0) goto werr;
            } else if (o->type == OBJ_STREAM) {
                if (rewriteStreamObject(aof,&key,o) == 0) goto werr;
            } else if (o->type == OBJ_MODULE) {
                if (rewriteModuleObject(aof,&key,o) == 0) goto werr;
            } else {
                serverPanic("Unknown object type");
            }
            /* Save the expire time */
            if (expiretime != -1) {
                char cmd[]="*3\r\n$9\r\nPEXPIREAT\r\n";
                if (rioWrite(aof,cmd,sizeof(cmd)-1) == 0) goto werr;
                if (rioWriteBulkObject(aof,&key) == 0) goto werr;
                if (rioWriteBulkLongLong(aof,expiretime) == 0) goto werr;
            }
            /* Read some diff from the parent process from time to time. */
            if (aof->processed_bytes > processed+AOF_READ_DIFF_INTERVAL_BYTES) {
                processed = aof->processed_bytes;
                aofReadDiffFromParent();
            }
        }
        dictReleaseIterator(di);
        di = NULL;
    }
    ...
}
```

Then most data of the whole redis instance are converted to aof records and save to
 file. Some infomation, like LRU and LFU of value, lua script cache won't be saved
 in aof records since they cannot be converted to normal command.

### References:
- [github: redis server.h](https://github.com/antirez/redis/blob/unstable/src/server.h)
- [github: redis aof.c](https://github.com/antirez/redis/blob/unstable/src/aof.c)
