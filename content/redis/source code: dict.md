<!---
tags: redis, dict, hash
-->

## redis: dict implementation

Dict is the key data structure in redis. A redis instance is like a huge centralized dict
 that can be shared by multiple application instances.

The dict in redis is a linked hash table. Compared to hash table in python,java, I
 think one of the biggest differences is rehashing. For map or hash table in program
 languages, they do will resize whole table and rehash all keys blocked. If the map is very
 large, it may block a long time. Luckily, it's very fast for small quantity of keys.

However, for redis, it often needs to deal with millions of keys. We cannot rehashing millions
 of keys at one time, it will lead to very large latency. Redis use two hash table for each
 dict and move keys from one to another step by step slowly. This may use more memory but
 it will avoid the latency problem.

### dict data structure
The `dictEntry` is used to store each key value pair. It uses a union here to deal with
 different value type. Since it is linked hash table. The `next` here is used to link another
 key value pair that has same hash value.

```c
// dict.h
typedef struct dictEntry {
    void *key;
    union {
        void *val;
        uint64_t u64;
        int64_t s64;
        double d;
    } v;
    struct dictEntry *next;
} dictEntry;
```

The dict also use some OOP methods. You can use the `dictType` to pass in your costomized
 hash function, compare function, constructor or desctructor of key and value.

```c
// dict.h
typedef struct dictType {
    uint64_t (*hashFunction)(const void *key);
    void *(*keyDup)(void *privdata, const void *key);
    void *(*valDup)(void *privdata, const void *obj);
    int (*keyCompare)(void *privdata, const void *key1, const void *key2);
    void (*keyDestructor)(void *privdata, void *key);
    void (*valDestructor)(void *privdata, void *obj);
} dictType;
```

A simple dictType usage from server.c:

```c
/* Command table. sds string -> command struct pointer. */
dictType commandTableDictType = {
    dictSdsCaseHash,            /* hash function */
    NULL,                       /* key dup */
    NULL,                       /* val dup */
    dictSdsKeyCaseCompare,      /* key compare */
    dictSdsDestructor,          /* key destructor */
    NULL                        /* val destructor */
};
```

The `dtctht` is definition of the hash table. The `table` points to beginning of the
 slots. It also stores size, sizemask and used slots.

```c
typedef struct dictht {
    dictEntry **table;
    unsigned long size;
    unsigned long sizemask;
    unsigned long used;
} dictht;
```

Each `dict` use two `dictht` to do rehashing incrementally. I will explain this later.
The `rehashidx` is used to record the progress.

```c
typedef struct dict {
    dictType *type;
    void *privdata;
    dictht ht[2];
    long rehashidx; /* rehashing not in progress if rehashidx == -1 */
    unsigned long iterators; /* number of iterators currently running */
} dict;
```

### dict create
The `dictCreate` will create a empty dict with size 0 and initiate values.

```c
dict *dictCreate(dictType *type,
        void *privDataPtr)
{
    dict *d = zmalloc(sizeof(*d));

    _dictInit(d,type,privDataPtr);
    return d;
}

/* Initialize the hash table */
int _dictInit(dict *d, dictType *type,
        void *privDataPtr)
{
    _dictReset(&d->ht[0]);
    _dictReset(&d->ht[1]);
    d->type = type;
    d->privdata = privDataPtr;
    d->rehashidx = -1;
    d->iterators = 0;
    return DICT_OK;
}
```

### find key value in dict
Just loop the two hash table to find a matched key and return the key value pair entry.

```c
dictEntry *dictFind(dict *d, const void *key)
{
    dictEntry *he;
    uint64_t h, idx, table;

    if (d->ht[0].used + d->ht[1].used == 0) return NULL; /* dict is empty */
    // each time you do find/delete/add, it will try to do one rehash step
    if (dictIsRehashing(d)) _dictRehashStep(d);
    h = dictHashKey(d, key);
    // loop over the two hash table
    for (table = 0; table <= 1; table++) {
        idx = h & d->ht[table].sizemask;
        he = d->ht[table].table[idx];
        // since it's linked hash table, need to loop all entries in same slot
        while(he) {
            if (key==he->key || dictCompareKeys(d, key, he->key))
                return he;
            he = he->next;
        }
        // if it's not in a rehashing, no need to loop the second hash table
        if (!dictIsRehashing(d)) return NULL;
    }
    return NULL;
}
```

The `_dictKeyIndex` is almost same as the `dictFind` except that it tries to get the
 slot index of the key and returns entry if existed.

### add key value to dict
This will try to add a new key if the key is not existed. The origin comments are very clear so that I didn't add more.

```c
dictEntry *dictAddRaw(dict *d, void *key, dictEntry **existing)
{
    long index;
    dictEntry *entry;
    dictht *ht;

    if (dictIsRehashing(d)) _dictRehashStep(d);

    /* Get the index of the new element, or -1 if
     * the element already exists. */
    if ((index = _dictKeyIndex(d, key, dictHashKey(d,key), existing)) == -1)
        return NULL;

    /* Allocate the memory and store the new entry.
     * Insert the element in top, with the assumption that in a database
     * system it is more likely that recently added entries are accessed
     * more frequently. */
    ht = dictIsRehashing(d) ? &d->ht[1] : &d->ht[0];
    entry = zmalloc(sizeof(*entry));
    entry->next = ht->table[index];
    ht->table[index] = entry;
    ht->used++;

    /* Set the hash entry fields. */
    dictSetKey(d, entry, key);
    return entry;
}
```

### upsert key value
The `int dictReplace(dict *d, void *key, void *val)` function will try to add the key
 value and it will update the value if key existed.

### delete key value
The `dictDelete` will delete the key value from the dict and free key value and the entry
 itself. While the `dictUnlink` will only delete key entry from the dict. It won't free
 the resource. Both of them will call the
 `dictEntry *dictGenericDelete(dict *d, const void *key, int nofree)`
 function underground. This will try to find the entry delete it.

### resize, expand
The `int dictExpand(dict *d, unsigned long size)` will try to expand the dict to a new
 size. It will round the size arguement to a power of 2. It just prepares a new hash
 table and set the `rehashidx` to 0. But it won't begin to rehash.

```c
// part of core code
    unsigned long realsize = _dictNextPower(size);

    n.size = realsize;
    n.sizemask = realsize-1;
    n.table = zcalloc(realsize*sizeof(dictEntry*));
    n.used = 0;

    d->rehashidx = 0;
```

The `int dictResize(dict *d)` will call dictExpand to resize the dict to its current
 used entries to make the entry/slot ratio less than 1.

### incremental rehashing
The `dictRehash(dict *d, int n)` try to do n steps rehashing. Each step will move a
 whole slot from ht[0] to ht[1].

There may have empty slots during the rehashing, it will skip quickly and try next
 one. In each rehashing phase, it will try to skip at most 10 * N empty stlos.

```c
// dictRehash core code
    while(n-- && d->ht[0].used != 0) {
        dictEntry *de, *nextde;

        /* Note that rehashidx can't overflow as we are sure there are more
         * elements because ht[0].used != 0 */
        assert(d->ht[0].size > (unsigned long)d->rehashidx);
        while(d->ht[0].table[d->rehashidx] == NULL) {
            d->rehashidx++;
            if (--empty_visits == 0) return 1;
        }
        de = d->ht[0].table[d->rehashidx];
        /* Move all the keys in this bucket from the old to the new hash HT */
        // you need to move all linked entries not only the head since they may have different hashes
        // e.g. value 7,15 go to same slot 7 when size is 8, then 15 will go to slot 16 when resize to 16
        while(de) {
            uint64_t h;

            nextde = de->next;
            /* Get the index in the new hash table */
            h = dictHashKey(d, de->key) & d->ht[1].sizemask;
            de->next = d->ht[1].table[h];
            d->ht[1].table[h] = de;
            d->ht[0].used--;
            d->ht[1].used++;
            de = nextde;
        }
        d->ht[0].table[d->rehashidx] = NULL;
        d->rehashidx++;
    }
```

As explained before, many CURD operations will try to do one step rehashing for the dict.
But it's not enough for the large main core dict. Then the redis server's `databaseCron`
 will choose one db to do one millinsecond's rehashing for its main dict or expire dict.


### References:
1. [github: redis dict](https://github.com/antirez/redis/blob/unstable/src/dict.c)
