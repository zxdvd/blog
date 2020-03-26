<!---
tags: redis, sds
-->

## redis: sds
The sds is a dynamic string module. You can use it to preallocate memory and append
 strings later to avoid too much malloc.

The author also made it a standalone repo at [github.com/antirez/sds](https://github.com/antirez/sds).
The README has very detailed introduction about it and many examples.

### definition
This module defines 5 subtype sds strings: sds_type_5, sds_type_8, sds_type_16, sds_type_32,
 sds_type_64. The numbers 5, 16 here mean length bits. sds_type_16 means its length
 is uint16_t so that it supports max length as 2^16-1.


Following is definition of the sdshdr16 and sdshdr64.
```c
struct __attribute__ ((__packed__)) sdshdr16 {
    uint16_t len; /* used */
    uint16_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};

struct __attribute__ ((__packed__)) sdshdr64 {
    uint64_t len; /* used */
    uint64_t alloc; /* excluding the header and null terminator */
    unsigned char flags; /* 3 lsb of type, 5 unused bits */
    char buf[];
};
```

For these subtypes, main difference is the size of fields `len` and `alloc`. I think this
 can help to save lots of memory since most time you only need small strings like sds_type_8
 or sds_type_16 so that no need to use the `uint64_t`.

The __attribute__ ((__packed__)) here is to tell GCC that no need to align struct members
. So there is no padding space between each members.

The `char buf[]` here is the so called [flexible array](https://en.wikipedia.org/wiki/Flexible_array_member).
I won't use any memory. You may just ignore it.


### sds sdsnewlen(const void *init, size_t initlen)
This will create a sds string that length is `initlen`. And you can use `init` to pass in the
 initial string.

```c
sds sdsnewlen(const void *init, size_t initlen) {
    void *sh;
    sds s;
    // get type 5,8,16,32,64 by initlen
    char type = sdsReqType(initlen);
    /* Empty strings are usually created in order to append. Use type 8
     * since type 5 is not good at this. */
    if (type == SDS_TYPE_5 && initlen == 0) type = SDS_TYPE_8;
    int hdrlen = sdsHdrSize(type);
    unsigned char *fp; /* flags pointer. */

    sh = s_malloc(hdrlen+initlen+1);
    if (init==SDS_NOINIT)
        init = NULL;
    else if (!init)
        memset(sh, 0, hdrlen+initlen+1);
    if (sh == NULL) return NULL;
    // s is the returned pointer, and it strips header from sh. So that it can be used as 
    // simple string without breaking the header
    s = (char*)sh+hdrlen;
    // since `buf` is ignored (flex array) and no padding. Then previous byte is the `flags`
    fp = ((unsigned char*)s)-1;
    switch(type) {
        // case 5, 16, 32 are ignored; set each member here
        case SDS_TYPE_8: {
            SDS_HDR_VAR(8,s);
            sh->len = initlen;
            sh->alloc = initlen;
            *fp = type;
            break;
        }
        case SDS_TYPE_64: {
            SDS_HDR_VAR(64,s);
            sh->len = initlen;
            sh->alloc = initlen;
            *fp = type;
            break;
        }
    }
    // copy the initial string if passed in; we can use the s as ordinary string
    if (initlen && init)
        memcpy(s, init, initlen);
    // initlen do not count the final `\0` (we got it using strlen)
    s[initlen] = '\0';
    return s;
}
```

### how to get the sds struct from the header stripped s
From agove we know that the returned `sds s` has no header, then how to get the header?
It has 5 types, then which type it belongs to?

Though the `len` and `alloc` fields are of different sizes. But the `flags` is fixed
 1 byte. And it is **the last member** (the buf is empty). Then we can get the type
 from char **s-1**.

After got the sds type, we know which struct to use and size of struct. Then we can unpack
 it like `structXX *sh = (char *)s - sizeof(structXX)` to get it.

```c
#define SDS_HDR(T,s) ((struct sdshdr##T *)((s)-(sizeof(struct sdshdr##T))))

static inline size_t sdslen(const sds s) {
    // s[-1] to get the previous byte
    unsigned char flags = s[-1];
    switch(flags&SDS_TYPE_MASK) {
        case SDS_TYPE_8:
            return SDS_HDR(8,s)->len;
        case SDS_TYPE_64:
            return SDS_HDR(64,s)->len;
    }
    return 0;
}
```

### new dup and free
The `sdsnew` will prepare a sds string with same length and copy the origin string.

```c
sds sdsnew(const char *init) {
    size_t initlen = (init == NULL) ? 0 : strlen(init);
    return sdsnewlen(init, initlen);
}
```

The `sdsdup` will duplicate a sds string. It is different to the `sdsnew`.

```c
sds sdsdup(const sds s) {
    return sdsnewlen(s, sdslen(s));
}
```

To free the sds string, need to get header size and calculated real pointer address to free.

```c
void sdsfree(sds s) {
    if (s == NULL) return;
    s_free((char*)s-sdsHdrSize(s[-1]));
}
```


### sds sdsMakeRoomFor(sds s, size_t addlen)
The `sdsMakeRoomFor` tries to enlarge the sds string so that we can append string to it
 later.

If the unused space is enough, it will return directly. Otherwise, it will create a new
 sds and copy string. It won't simply extend extra addlen length, it will try to add more
 since you may append multiple times.

```c
    newlen = (len+addlen);
    if (newlen < SDS_MAX_PREALLOC)
        newlen *= 2;
    else
        newlen += SDS_MAX_PREALLOC;
```


### long long to sds
The `int sdsll2str(char *s, long long value)` will convert long long type value to string
 and store at s (no malloc inside this function). The `sdsull2str` is almost the same that
 it converts the unsigned long long value.

It will convert from lowest side that number 1234 will convert to `4321` and then it will
 do a inplace reverse so that we get string `1234` finally.

The `sds sdsfromlonglong(long long value)` will use the sdsll2str to do converting and return
 a sds string.


### References:
1. [github: redis sds.h](https://github.com/antirez/redis/blob/unstable/src/sds.h)
2. [github: redis sds.c](https://github.com/antirez/redis/blob/unstable/src/sds.c)
3. [gcc doc: __attribute__](https://gcc.gnu.org/onlinedocs/gcc-4.0.2/gcc/Type-Attributes.html)

