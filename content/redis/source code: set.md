```metadata
tags: redis, sourcecode, set
```

## redis set

It's easy to understand that `set` can use `dict` as the underground data structure.
 However, redis uses `intset` data structure if all the keys can be encoded as integer.
 It's more dense than `dict` and can save memory. It will convert to `dict` if the
 size grows to a threshold.

THINK: I think maybe compressed bitmap can replace `intset` to achieve better performance
 and use less memory.

### create set
If the first key can be encoded as integer, it will use `intset`, otherwise `dict` is used.

```c
robj *setTypeCreate(sds value) {
    if (isSdsRepresentableAsLongLong(value,NULL) == C_OK)
        return createIntsetObject();
    return createSetObject();
}
```

### add and remove keys
Since `set` can be encoded as two different data structures, it need to check container
 type and then use related low level functions.

And since `intset` can only hold integer, then it has to be converted to normal `dict`
 if you add a non-integer later after `intset` created. It will also convert to `dict`
 if length of the `intset` is greater than `server.set_max_intset_entries` (default
 512 and can be configed) since operations on `intset` are often O(n) complexity then
 performance is bad if size is too large.

Attention: you'd better avoid using empty string, `null` in the set if most elements
 are integers.

```c
int setTypeAdd(robj *subject, sds value) {
    long long llval;
    if (subject->encoding == OBJ_ENCODING_HT) {
    // add element to dict
    } else if (subject->encoding == OBJ_ENCODING_INTSET) {
        if (isSdsRepresentableAsLongLong(value,&llval) == C_OK) {
            uint8_t success = 0;
            subject->ptr = intsetAdd(subject->ptr,llval,&success);
            if (success) {
                /* Convert to regular set when the intset contains too many entries. */
                if (intsetLen(subject->ptr) > server.set_max_intset_entries)
                    setTypeConvert(subject,OBJ_ENCODING_HT);
                return 1;
            }
        } else {
            /* Failed to get integer from object, convert to regular set. */
            setTypeConvert(subject,OBJ_ENCODING_HT);
            // add to dict
        }
    } else {
        serverPanic("Unknown set encoding");
    }
    return 0;
}
```

### high level sadd command
Above is low level `set` operations. For high level command, like `sadd`, it first
 find the key from the global dict and check its type. Then it calls the low level
 `setTypeAdd` to add elements.

```c
void saddCommand(client *c) {
    robj *set;
    int j, added = 0;

    set = lookupKeyWrite(c->db,c->argv[1]);
    if (set == NULL) {
        set = setTypeCreate(c->argv[2]->ptr);
        dbAdd(c->db,c->argv[1],set);
    } else {
        // check the object type
        if (set->type != OBJ_SET) {
            addReply(c,shared.wrongtypeerr);
            return;
        }
    }
    // call setTypeAdd to add all elements one by one
    for (j = 2; j < c->argc; j++) {
        if (setTypeAdd(set,c->argv[j]->ptr)) added++;
    }
    if (added) {
        signalModifiedKey(c->db,c->argv[1]);
        notifyKeyspaceEvent(NOTIFY_SET,"sadd",c->argv[1],c->db->id);
    }
    server.dirty += added;
    addReplyLongLong(c,added);
}
```

### set union and diff
The function
`void sunionDiffGenericCommand(client *c, robj **setkeys, int setnum, robj *dstkey, int op) {}`
 deals with set union and diff.

Union is very simple, the pseudo code is like following:

```js
    const result = new Set()
    for (const s of sets) {
        for (const element of s) {
            result.add(element)
        }
    }
    // deal with result according to dstkey (either store to the key or return to client)
```

For `diff`, it has two algorithms. Algorithm 1 is O(N*M) while N is size of first set
 and M is total number of sets. The pseudo code:

```js
    const result = new Set()
    for (const element of firstSet) {
        for (const s of otherSets) {
            // if the element is in any of the other set, then it should be in result of the diff
            if (s.has(element)) {
                continue
            }
        }
    }
```

Algorithm 2 is O(N) where N is total number of elements in all the set. The pseudo code:

```js
    const result = new Set()
    // copy all of first set to result
    for (const element of firstSet) {
        result.add(element)
    }
    // remove all element in other sets from the result set, then the left is the diff
    for (const s of otherSets) {
        for (const element of s) {
            result.delete(element)
        }
    }
```

Redis will compare the complexity of the two algorithms according to the actually set
 size and set numbers and choose the right one.

### set intersection
The `sinter` get intersection elements of all the sets. The algorithm is simple, it first
 sorts the sets by set size to get set of smallest size. Then it iterates the smallest set,
 and check whether it exists in **ALL** other sets or not. The pseudo code:

```js
    outer: for (const element of smallestSet) {
        for (const s of otherSets) {
            // if any set DO NOT has this element, skip it
            if (!s.has(element)) {
                continue outer
            }
        }
        // all other sets has this element, then write to client or store to dstkey
    }
```

### references
- [github: redis t_set.c](https://github.com/redis/redis/blob/5.0.0/src/t_set.c)

