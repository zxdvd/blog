```metadata
tags: redis, sourcecode, aof
```

## redis: aof states
The AOF has a lot of global settings and states and I'll analyze them at this post.

### states definition
Following is AOF related states, rewrite related is ignored.

```c
struct redisServer {
    ...
    /* AOF persistence */
    int aof_enabled;                /* AOF configuration */
    int aof_state;                  /* AOF_(ON|OFF|WAIT_REWRITE) */
    int aof_fsync;                  /* Kind of fsync() policy */
    char *aof_filename;             /* Name of the AOF file */
    off_t aof_current_size;         /* AOF current size. */
    off_t aof_fsync_offset;         /* AOF offset which is already synced to disk. */
    sds aof_buf;      /* AOF buffer, written before entering the event loop */
    int aof_fd;       /* File descriptor of currently selected AOF file */
    int aof_selected_db; /* Currently selected DB in AOF */
    time_t aof_flush_postponed_start; /* UNIX time of postponed AOF flush */
    time_t aof_last_fsync;            /* UNIX time of last fsync() */
    unsigned long aof_delayed_fsync;  /* delayed AOF fsync() counter */
    int aof_last_write_status;      /* C_OK or C_ERR */
    int aof_last_write_errno;       /* Valid if aof_last_write_status is ERR */
    int aof_load_truncated;         /* Don't stop on unexpected AOF EOF. */
    ...
}
```

### AOF settings
The `aof_enabled` is loaded from config `appendonly`. And the `aof_state` is initialized
 from `aof_enabled`. The `aof_state` may be changed in some situations, like replication
 and aof rewrite.

```c
    server.aof_state = server.aof_enabled ? AOF_ON : AOF_OFF;
```

The `server.aof_fsync` is the fsync policy. Currently, it can use one of `everysec`,
 `always` and `no`. And `everysec` is the default and suggested policy.

`always` means that it will do fsync everytime the `flushAppendOnlyFile` is called.
`everysec` means it will try to do fsync every second while `no` means that let the OS
 managing the fsync.

### AOF states
The `aof_buf` is a sds string that acts as a temporary buffer for AOF records. It will
 be flush into the aof file periodically.

The `aof_fd` is file descriptor of the opened aof file.

The `aof_selected_db` is the selected db of last aof record. Without this, each aof record
 may need a new field to mark which db the record belongs to since redis can have 16 dbs.

With this, if next record has a different dbid, redis will generate a `select dbid` command
 and append it first. This has a drawback, if records switch between multiple dbs too
 frequently, it will lead to too much `select dbid` records.

The `aof_current_size` and `aof_fsync_offset` are easy to understand. A successful write
 will increase the `aof_current_size`. And a fsync will sync data to disk and after a
 successful fsync the `aof_fsync_offset` will be set equal to `aof_current_size`.

The `aof_current_size` can be used to withdraw a short write. After a short write, redis
 will truncate file to the `aof_current_size` then short write has no effect to file.

The `aof_last_fsync` is timestamp (in second) of last fsync. Redis uses this to compared
 with server.unixtime to decide that whether a fsync is needed.

By default the `aof_load_truncated` is enabled, so that if partial record is read at end
 of file, redis will drop the partial record and truncate file size to the last good
 record. A failed fsync or crash may lead to partial record.

The fsync may be delayed due to another thread is doing async fsync. The `aof_delayed_fsync`
 is a counter for this delay. You can get this from `info` command. If this counter
 becomes large then maybe the system has trouble with IO.

The `aof_flush_postponed_start` is timestamp of the postponed fsync.

The `aof_last_write_status` and `aof_last_write_errno` are easy to understand. Redis will
 retry fsync will `serverCron` if last fsync failed. And it will also deny write command.

### References:
- [github: redis server.h](https://github.com/antirez/redis/blob/unstable/src/server.h)
- [github: redis aof.c](https://github.com/antirez/redis/blob/unstable/src/aof.c)
