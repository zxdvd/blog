```metadata
tags: linux, memory, malloc, tcmalloc
```

## tcmalloc introduction

`tcmalloc` is a high performance replacement of `malloc` and related `free`. `tc` refers
 to `Thread Caching`.

I'll explain how `malloc` works at first and then go through how `tcmalloc` achieve high
 performance.

### general malloc
`malloc` can extend or reserve virtual memory space from kernel so that application can
 use malloced addresses. It needs to maitain the use/free state of each pointer or address
 range. The data structure could be a rbtree or others. To malloc a size, it needs to
 traverse the tree to find an enough free space.

Then `malloc` can be bottleneck of your system due to following reasons:
- it needs lock to protect the state tree (or other data structure) since multiple threads
 share a same virutal memory space. And lock may decrease performance a lot

- both reserve memory and free memory are system calls; system calls are expensive sometimes

- it's O(n) to traverse a tree find a free space of enough size

- frequently malloc and free may lead to a lot of fragments in memory space and then it's
 not easy to find a large enough space

### tcmalloc features
`tcmalloc` will pre-alloc pages and separate them to groups of different size classes. A
 size class can be 8 bytes, 16 bytes, 24 bytes to 4k bytes. When you need to malloc a piece
 of memory, it will be rounded to nearest size class and `tcmalloc` will find one from
 the group of this size class. It is returned to the group when freed.

But where is `Thread Caching`?

`tcmalloc` used to prepare caches of different size classes groups for each thread. So that
 malloc in each thread only needs to interact with the thread local data structure. No locks.

What about caches are used out or requested size is too large? For this situation, it will
 request more from central caches or allocate from system directly (if too large). It needs
 lock for this case.

For most applications, small size malloc happens much more frequently. So for most cases, the
cache is enough and you avoid locks and system calls. And separating different size classes
 also reduces fragments of virtual memory space.

More recently `tcmalloc` implements the `per-cpu` mode based on `restart sequence` (need 
kernel >= 4.18). Some applications may have a lot of threads and the `per-thread` caches will
 use too much memory by themselves. The `per-cpu` mode can avoid this. This is the default mode
 now.

### caveats
`tcmalloc` also has some caveats that you need to know

- metadata also uses a lot of memory, especially when you have many threads or cpus
- requested size is rounded to nearest size class and this may waste some memory
- pre-allocate leads to large VSS (RSS is not affected since they are not page faulted)

### references
-. [tcmalloc design](https://github.com/google/tcmalloc/blob/master/docs/design.md)
