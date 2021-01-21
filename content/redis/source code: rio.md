```metadata
tags: redis, sourcecode, rio
```

## redis: rio

The `rio` module is an abstract to some low level objects: buffer, file, socket and
 file descriptor. It exposes uniform api like `rioWrite`, `rioRead`, `rioTell` and
 `rioFlush`.

### data structure

```c
// redis: src/rio.h
struct _rio {
    /* Backend functions.
     * Since this functions do not tolerate short writes or reads the return
     * value is simplified to: zero on error, non zero on complete success. */
    size_t (*read)(struct _rio *, void *buf, size_t len);
    size_t (*write)(struct _rio *, const void *buf, size_t len);
    off_t (*tell)(struct _rio *);
    int (*flush)(struct _rio *);
    void (*update_cksum)(struct _rio *, const void *buf, size_t len);

    /* The current checksum and flags (see RIO_FLAG_*) */
    uint64_t cksum, flags;

    /* number of bytes read or written */
    size_t processed_bytes;

    /* maximum single read or write chunk size */
    size_t max_processing_chunk;

    /* Backend-specific vars. */
    union {
        /* In-memory buffer target. */
        struct {
            sds ptr;
            off_t pos;
        } buffer;
        /* Stdio file pointer target. */
        struct {
            FILE *fp;
            off_t buffered; /* Bytes written since last fsync. */
            off_t autosync; /* fsync after 'autosync' bytes written. */
        } file;
        /* Connection object (used to read from socket) */
        struct {
            connection *conn;   /* Connection */
            off_t pos;    /* pos in buf that was returned */
            sds buf;      /* buffered data */
            size_t read_limit;  /* don't allow to buffer/read more than that */
            size_t read_so_far; /* amount of data read from the rio (not buffered) */
        } conn;
        /* FD target (used to write to pipe). */
        struct {
            int fd;       /* File descriptor. */
            off_t pos;
            sds buf;
        } fd;
    } io;
};

typedef struct _rio rio;
```

The `struct _rio` defines 5 function pointers that point to low level api of each
 actually io type. The `io` field is a union of 4 supported io types.


### rio buffer
The `buffer IO` uses a `sds` string underground as buffer. And you can read from this
 buffer or write to it. It is used in the `dumpCommand` (DUMP key).

```c
/* Returns 1 or 0 for success/failure. */
static size_t rioBufferWrite(rio *r, const void *buf, size_t len) {
    r->io.buffer.ptr = sdscatlen(r->io.buffer.ptr,(char*)buf,len);
    r->io.buffer.pos += len;
    return 1;
}

static size_t rioBufferRead(rio *r, void *buf, size_t len) {
    if (sdslen(r->io.buffer.ptr)-r->io.buffer.pos < len)
        return 0; /* not enough buffer to return len bytes. */
    memcpy(buf,r->io.buffer.ptr+r->io.buffer.pos,len);
    r->io.buffer.pos += len;
    return 1;
}

/* Returns read/write position in buffer. */
static off_t rioBufferTell(rio *r) {
    return r->io.buffer.pos;
}

/* Flushes any buffer to target device if applicable. Returns 1 on success
 * and 0 on failures. */
static int rioBufferFlush(rio *r) {
    UNUSED(r);
    return 1; /* Nothing to do, our write just appends to the buffer. */
}
```

It implements the read, write, tell and flush low level apis and then use it to make
 a prototype structure `rioBufferIO`.

And the `rioInitWithBuffer` is used to create a `rio` object from a `sds` string.
 It copies members from the `rioBufferIO` prototype and sets pointer position to 0.

```c
static const rio rioBufferIO = {
    rioBufferRead,
    rioBufferWrite,
    rioBufferTell,
    rioBufferFlush,
    NULL,           /* update_checksum */
    0,              /* current checksum */
    0,              /* flags */
    0,              /* bytes read or written */
    0,              /* read/write chunk size */
    { { NULL, 0 } } /* union for io-specific vars */
};

void rioInitWithBuffer(rio *r, sds s) {
    *r = rioBufferIO;
    r->io.buffer.ptr = s;
    r->io.buffer.pos = 0;
}
```

We may find that `rioBufferFlush` seems do nothing. Thus because it is an in-memory
 buffer. It doesn't have a backed on disk file so no need to flush.

And the `Connection` rio type has a similar function `rioConnWrite`. It just returns
 error since redis only uses it in reading RDB file from socket nowhere uses the `write`
 api of `Connection` rio.

### connection rio
The `Connection` rio type has an internal buffer. I think redis mainly uses it to avoid
 the short read. It has an internal buffer so that it can either returns 0 byte or
 requested bytes if the buffer gets enough bytes from socket.

### rioWrite and rioRead
`rioWrite` and `rioRead` are two high level functions that use will repeat the read
 or write until the specific bytes are processed.

```c
static inline size_t rioWrite(rio *r, const void *buf, size_t len) {
    if (r->flags & RIO_FLAG_WRITE_ERROR) return 0;
    while (len) {
        size_t bytes_to_write = (r->max_processing_chunk && r->max_processing_chunk < len) ? r->max_processing_chunk : len;
        if (r->update_cksum) r->update_cksum(r,buf,bytes_to_write);
        if (r->write(r,buf,bytes_to_write) == 0) {
            r->flags |= RIO_FLAG_WRITE_ERROR;
            return 0;
        }
        buf = (char*)buf + bytes_to_write;
        len -= bytes_to_write;
        r->processed_bytes += bytes_to_write;
    }
    return 1;
}
```

### references
- [github: redis rio.c](https://github.com/redis/redis/blob/5.0.0/src/rio.c)


