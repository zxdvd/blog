<!---
tags: read-code, c, redis, malloc
-->

While most programs will use malloc from libc, there are other high performance
 allocator like jemalloc, tcmalloc.

Redis will use jemalloc on linux if it's not an ARM, otherwise it will use libc's.
We can get this from the Makefile

```
# Default allocator defaults to Jemalloc if it's not an ARM
MALLOC=libc
ifneq ($(uname_M),armv6l)
ifneq ($(uname_M),armv7l)
ifeq ($(uname_S),Linux)
	MALLOC=jemalloc
endif
endif
endif
```

The zmalloc module is a lightweight wrapper around these different allocators.
And it implements the `zmalloc_size` function if the allocator doesn't support it.

### malloc_size
This is a function to get size of allocated memory of a pointer. Macos supports
 it. `tc_malloc_size` of tcmalloc and `je_malloc_usable_size` of jemalloc also
 implement this. However, some allocator may not have this.

Then redis implements its own zmalloc_size as a fallback. How to implement it?
The magic is hiding the size at the beginning of the allocated memory.

If you have allocated 1000 bytes at address 10000, then use the first 4 bytes
 (may also be 8 bytes on 64 bits system) to store the size. And return 10004 as
 address to the caller. It has no effect to the caller.

When free the pointer, you need to minus 4 bytes to get the real address to free.
And remeber you must not use the origin `free` to free the pointer. You need the
 customized one that will free the real address.

By this way, you can always get the allocated size from the 4 bytes hiding before
 the pointer.

### memory stat
There is other benefits when using a customized allocator. You can store the used
 memory and know how much memory the program used.

The `zmalloc` also has some helper functions to extract other memory info from the
 proc file system.

### Reference:
1. [github: redis zmalloc](https://github.com/antirez/redis/blob/unstable/src/zmalloc.c)
