```metadata
tags: redis, sourcecode, set, intset
```

## redis intset

From article [redis set](./source code: set.md) we know that redis uses `intset` to
 store integers when the size of set is limit.

`intset` is actually a sorted array of integers. It uses less memory than dict. Though
 it is O(lgN) but considering it is simple, small and compact, it is still very fast.

Data structure of `intset`:

```c
typedef struct intset {
    uint32_t encoding;
    uint32_t length;
    int8_t contents[];
} intset;
```

`intset` supports 3 encoding: INT16, INT32 and INT64. It is decided by the smallest
 element or the largest element. If current encoding is INT16 and you'll add an INT64,
 then it has to convert the whole array from INT16 to INT64.

The `int8_t contents[]` is actually a pointer to the array. For any operation on the
 array, you need to know the encoding first.

Let's see the function of indexing by position:

```c
static int64_t _intsetGetEncoded(intset *is, int pos, uint8_t enc) {
    int64_t v64;
    int32_t v32;
    int16_t v16;

    if (enc == INTSET_ENC_INT64) {
        memcpy(&v64,((int64_t*)is->contents)+pos,sizeof(v64));
        memrev64ifbe(&v64);
        return v64;
    } else if (enc == INTSET_ENC_INT32) {
        // similar to v64
    } else {
        // similar to v64
    }
}
```

You need to convert the pointer according to the encoding to get value by position.
 You can also do it via `(char *)is->contents + pos * sizeof(v64)`.

You may wonder that why it uses `memrev64ifbe` here. `be` means `big endian`. Redis
 supports many different OS and architectures (x86, arm). And it supports persistence.
 The saved database may be transferred to another architecture with different endian.
 You'll get wrong data from multi-bytes data type like int16, int32. To avoid this,
 redis will convert it to little endian first and then store in memory. So it means
 that the database is always little endian. And for big endian architecture, it needs
 to convert data fetched from database to little endian using functions like `memrev64ifbe`.

For set operation, it is similar to the get operation. For searching, it is just
 a simple binary search. For binary search, a common mistake is `mid = (min+max)/2`.
 Attention that `min+max` can get overflowed. You should use `mid = min/2 + max/2`
 or using the redis way: convert int to `unsigned int` to avoid overflowing.

```c
        mid = ((unsigned int)min + (unsigned int)max) >> 1;
```

### references
- [github: redis t_set.c](https://github.com/redis/redis/blob/5.0.0/src/intset.c)
