```metadata
tags: linux, io, read, write
```

## read write apis

A introduction of following apis:

    read/write
    pread/pwrite
    readv/writev
    preadv/pwritev
    fread/fwrite

### read/write
`read` and `write` are the most common io apis.

```c
#include <unistd.h>
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
```

The returned value is the actual read/write size. It may be less than `count` which
 indicates a short read or short write happened. For `read`, 0 returned means it reaches
 EOF (end of file).

### pread/pwrite
Sometimes, you may need to read/write at a specific offset. You can achieve it via
 `lseek` and then read/write. But that's two system calls. You can achieve same effect
 via `pread/pwrite`.

```c
 #include <unistd.h>
ssize_t pread(int fd, void *buf, size_t count, off_t offset);
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
```

Attention, `pread/pwrite` won't update offset of file descriptor while `read/write` will.

### readv/writev and preadv/pwritev
The `v` in `readv/writev` means **vector**. They can read/write a vector of buffers to a
 file descriptor sequentially in just one system call atomically. Of course, you can do
 it in userspace using a for loop of `read/write`. But that will use too much system calls
 and is not atomic.

```c
 #include <sys/uio.h>
ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
ssize_t preadv(int fd, const struct iovec *iov, int iovcnt, off_t offset);
ssize_t pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset);

struct iovec {
    void  *iov_base;    /* Starting address */
    size_t iov_len;     /* Number of bytes to transfer */
};
```

The `iovcnt` is length of vector. It's easy to understand that you may get short read/write.
But attention, the short read/write may lead to that a single `iovec` is partially read or
 write. So you need to check the returned read/write **bytes** to deal with short read/write.

The `preadv/pwritev` is combination of `pread/pwrite` and `readv/writev`. It supports an
 extra offset argument.


### fread/fwrite
`fread/fwrite` are different from above io apis. They are not system calls. They are userspace
 utils implemented in libc. They can read/write buffer to an array of fixed size element. They
 return number of items read or write. Attention here, not bytes but number of items.

```c
 #include <stdio.h>
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
```

They are very useful in the case that you don't want to deal with a partial read/write struct.
You may get short read or write. But the minimum unit is item but not byte. There is no short
 read/write of a single item.

A example from redis as following. It uses `len` as size while item number is **1**. So that
 it either returns 0 which means write nothing or returns 1 means that ALL `len` bytes are
 read.

```c
// redis: src/rio.c
static size_t rioFileRead(rio *r, void *buf, size_t len) {
    return fread(buf,len,1,r->io.file.fp);
}
```

It's simple to read/write a fixed length of buffer either succeeding or failing. No partial
 read or write.

Attention

- `fread/fwrite` are not system calls, they maintain internal buffers. So don't mix them
 with `read/write` for a same fd.
- the `no partial read/write` of a struct is only meaningful for `fread/fwrite` api level,
 it may read/write some bytes at OS level


