<!---
tags: redis, source code, type
-->

From post [redis data types](./data types and internal types.md) we know that a redis
 key may have a value of any type. Then it needs to store the type info in the value.
Consider that each type may have different internal types (hash can be stored in dict
 or ziplist). Then you need a subtype. So an object struct is needed:

```c
// server.h
typedef struct redisObject {
    unsigned type:4;
    unsigned encoding:4;
    unsigned lru:LRU_BITS; /* LRU time (relative to global lru_clock) or
                            * LFU data (least significant 8 bits frequency
                            * and most significant 16 bits access time). */
    int refcount;
    void *ptr;
} robj;

/* The actual Redis Object */
#define OBJ_STRING 0    /* String object. */
#define OBJ_LIST 1      /* List object. */
#define OBJ_SET 2       /* Set object. */
#define OBJ_ZSET 3      /* Sorted set object. */
#define OBJ_HASH 4      /* Hash object. */

#define OBJ_ENCODING_RAW 0     /* Raw representation */
#define OBJ_ENCODING_INT 1     /* Encoded as integer */
#define OBJ_ENCODING_HT 2      /* Encoded as hash table */
#define OBJ_ENCODING_ZIPMAP 3  /* Encoded as zipmap */
#define OBJ_ENCODING_LINKEDLIST 4 /* No longer used: old list encoding. */
#define OBJ_ENCODING_ZIPLIST 5 /* Encoded as ziplist */
#define OBJ_ENCODING_INTSET 6  /* Encoded as intset */
#define OBJ_ENCODING_SKIPLIST 7  /* Encoded as skiplist */
#define OBJ_ENCODING_EMBSTR 8  /* Embedded sds string encoding */
#define OBJ_ENCODING_QUICKLIST 9 /* Encoded as linked list of ziplists */
#define OBJ_ENCODING_STREAM 10 /* Encoded as a radix tree of listpacks */
```

The encodings act as the subtypes. There is a `lru` field in the `robj` that is used
 to log recent access time or frequency. This info is very useful to the evicting
 strategy.

The `refcount` name is very clear. It records reference count of an object. A object
 can be shared in db's main dict, db's expire dict, client buffers and other places.
So it needs a `refcount` to log it and free an object if it isn't used at any place.

There is a special value for `refcount`, it is `OBJ_SHARED_REFCOUNT` (INT_MAX). It
 means the object is shared and no need to add/substract it. It is used for small
 integers that will be used very frequently.For example, 0, 1, 2.

```c
// server.c   OBJ_SHARED_INTEGERS is 10000 by default
    for (j = 0; j < OBJ_SHARED_INTEGERS; j++) {
        shared.integers[j] =
            makeObjectShared(createObject(OBJ_STRING,(void*)(long)j));
        shared.integers[j]->encoding = OBJ_ENCODING_INT;
    }
```

### string object
If the string length is less than a limit (default 44), it will be embedded into the
 object (allocated with the object together and stored after the object). This will
 have better performance since string and object in same cache line, no need to fetch
 string from memory.

```c
#define OBJ_ENCODING_EMBSTR_SIZE_LIMIT 44
robj *createStringObject(const char *ptr, size_t len) {
    if (len <= OBJ_ENCODING_EMBSTR_SIZE_LIMIT)
        return createEmbeddedStringObject(ptr,len);
    else
        return createRawStringObject(ptr,len);
}
```

### integer object
For integer, if it is in the range of the shared integers (default 0~10000), then the
 shared object will be used (no allocation). Otherwise, if it is in the range of
 `LONG INT`, it will be saved in the `obj->ptr`, for this case, the pointer is actually
 a `LONG INT`, you must not deference the pointer. If it cannot fit into `LONG INT`,
 it is fallback to store using sds string.

```c
            o = createObject(OBJ_STRING,sdsfromlonglong(value));
```

### constructor and destructor
For each encoding, a constructor is defined. A destructor is defined for each type.

```c
robj *createQuicklistObject(void) {}
robj *createZiplistObject(void) {}

void freeListObject(robj *o) {}
void freeSetObject(robj *o) {}
```

### misc
There is a lot of memory related functions in object.c but I won't walk through them now.

### References:
1. [github: redis object](https://github.com/antirez/redis/blob/unstable/src/object.c)
