<!---
tags: redis, data structure, quicklist
-->

## redis: quicklist data structure

Quicklist is double linked list. The payload is ziplist that may be compressed.
The main use is for redis **list**. You may wonder that why linked list, why not use
 ziplist directly.

- you may need allocate if free space in ziplist is not enough to hold new values
- you need to do memmove if insert at head or middle

For small list, it's ok to do allocation or memory moving, but for very large list,
 this will decrease performance largely.

Then the `quicklist` partitions a large list to many small `ziplist` nodes. Then
 you only allocate or memmove a single ziplist node.

### definition
Following is definition for the quicklist, quicklistNode. The origin comments are very
 clear.

Both the quicklist and node have `count` to record total items in list or node.

```c
typedef struct quicklist {
    quicklistNode *head;
    quicklistNode *tail;
    unsigned long count;        /* total count of all entries in all ziplists */
    unsigned long len;          /* number of quicklistNodes */
    int fill : 16;              /* fill factor for individual nodes */
    unsigned int compress : 16; /* depth of end nodes not to compress;0=off */
} quicklist;

typedef struct quicklistNode {
    struct quicklistNode *prev;
    struct quicklistNode *next;
    unsigned char *zl;           // ziplist
    unsigned int sz;             /* ziplist size in bytes */
    unsigned int count : 16;     /* count of items in ziplist */
    unsigned int encoding : 2;   /* RAW==1 or LZF==2 */
    unsigned int container : 2;  /* NONE==1 or ZIPLIST==2 */
    unsigned int recompress : 1; /* was this node previous compressed? */
    unsigned int attempted_compress : 1; /* node can't compress; too small */
    unsigned int extra : 10; /* more bits to steal for future usage */
} quicklistNode;
```

### push head, push tail
To push a value at beginning, it first check whether the head node's ziplist is allowed
 to push. If yes, just push it at beginning of the ziplist. Otherwise, it will create
 a new node and push this value to the node, then insert this new node to the linked
 list as head node.

It will update count of quicklist and node finally.

To push at tail is almost same head.

```c
int quicklistPushHead(quicklist *quicklist, void *value, size_t sz) {
    quicklistNode *orig_head = quicklist->head;
    if (likely(
            _quicklistNodeAllowInsert(quicklist->head, quicklist->fill, sz))) {
        quicklist->head->zl =
            ziplistPush(quicklist->head->zl, value, sz, ZIPLIST_HEAD);
        quicklistNodeUpdateSz(quicklist->head);
    } else {
        quicklistNode *node = quicklistCreateNode();
        node->zl = ziplistPush(ziplistNew(), value, sz, ZIPLIST_HEAD);

        quicklistNodeUpdateSz(node);
        _quicklistInsertNodeBefore(quicklist, quicklist->head, node);
    }
    quicklist->count++;
    quicklist->head->count++;
    return (orig_head != quicklist->head);
}
```

### allow insert and fill
We know that quicklist is linked ziplist. Then how large should a ziplist node be?

If the node is too big, reallocation and memmoving will be slow. If the node is too
 small, there may have too many nodes and it falls to like a simple linked list and
 is not so memory saving.

So redis has a few rules about a large a node should be. There is a important field
 `fill` in quicklist. Its value range is -5 to 2^15.

If the `fill` value is between -5 to -1, it has defined size allow to hold.
By default, the `fill` is -2 when created. It means 8KB is allowed. -5 means 54KB
 allowed.

```c
static const size_t optimization_level[] = {4096, 8192, 16384, 32768, 65536};
```

Following is logic of allow insert. First check with optimization_level then check
 if more than 8KB and then node count.

```c
_quicklistNodeSizeMeetsOptimizationRequirement(const size_t sz,
                                               const int fill) {

    size_t offset = (-fill) - 1;
    if (offset < (sizeof(optimization_level) / sizeof(*optimization_level))) {
        if (sz <= optimization_level[offset]) {
            return 1;
        } else {
            return 0;
        }
    }
}

// SIZE_SAFETY_LIMIT defined as 8192 (8k)
#define sizeMeetsSafetyLimit(sz) ((sz) <= SIZE_SAFETY_LIMIT)

REDIS_STATIC int _quicklistNodeAllowInsert(const quicklistNode *node,
                                           const int fill, const size_t sz) {
    ....
    unsigned int new_sz = node->sz + sz + ziplist_overhead;
    if (likely(_quicklistNodeSizeMeetsOptimizationRequirement(new_sz, fill)))
        return 1;
    else if (!sizeMeetsSafetyLimit(new_sz))    // not allowed if more than 8k
        return 0;
    else if ((int)node->count < fill)         // allowed if node count less than fill
        return 1;
    else
        return 0;
}
```

### insert in the middle
The function `_quicklistInsert` deals with insert in the middle of a quicklist.
It will insert the `value` before or after the specific `entry`. The `after` parameter
 determines before or after.

```c
REDIS_STATIC void _quicklistInsert(quicklist *quicklist, quicklistEntry *entry,
                                   void *value, const size_t sz, int after) {}
```

It have following cases:

#### the entry has a null node (not in the list)
It will create a new node with single value.

#### node is not full
This case is easy, just call ziplist related methods to insert the value into ziplist.

#### node is full
So we cannot simple insert it. This case can be divided into following two cases

- insert at end and the next node is not full

    For this case, it now turns into `insert at beginning of the next not full node`

- insert at beginning and previous node is not full

    This turns to be `insert at end of the previous not full node`

- insert at end and the next node is also full

    For this case, just create a new node and add this new node into the quicklist.

- insert at beginning and previous node is also full

    Same as above, just create a new node.

- insert at middle of a full node

    For this case, we need to split the node at the entry and then insert value.


### node compression and depth
We often use redis to store json data or html, mostly text. And the compression ratio
 for text can be perfectly good. For example, a 100MB text can be compessed to use
 only 5MB to 10MB.

Then we could save a lot of memory if we compressed each node of the quicklist. However,
 everything is trade off.

- We'll use more CPU to compress and decompress.
- We need to decompress, update and then compress each time we need to change the ziplist

Considering that the most frequent operations of list may be push at head or tail. Then
 we can set that skipping compression of N nodes at each side.

For example, if we set the `quicklist->compress` as 1, it means the skipping head and
 tail. If we set it to 2, skipping first two and last two.

The default setting is 0 while create a quicklist.


### References:
- [github: redis quicklist](https://github.com/antirez/redis/blob/unstable/src/quicklist.c)
- [detailed blog: redis quicklist visions](https://matt.sh/redis-quicklist-visions)
