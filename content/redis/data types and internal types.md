```metadata
tags: redis, data type
```

## redis data types and internal types

We know that redis following main data types:

| outer type         | internal type              |
| ------------------ | -------------------------- |
| string             | sds, embedded sds          |
| list               | quicklist                  |
| set                | intset / dict              |
| zset (ordered set) | ziplist / (skiplist, dict) |
| hash               | ziplist / dict             |


Each data type may use one or more internal type to store.

For string, it will use sds string.

For list, it uses `quicklist` internally.

For set, it may use `intset` to save memory if all values are integer. Otherwise, it
 will use `dict`.

For zset, it will use `ziplist` to save memory for small size. When it became larger,
 it will convert to use `skiplist`.

For hash ,it will use `ziplist` for small size and convert to use `dict`.

If we think about internal data structure first, we can get following table:

| internal type | property               | outer type      |
| ------------- | ---------------------- | --------------- |
| sds           |                        | string          |
| quicklist     | ordered, linked        | list            |
| skiplist      | skiplist, ordered      | zset            |
| dict          | hash table             | set, hash, zset |
| ziplist       | binary string, ordered | zset, hash      |
| intset        | binary, ordered        | set             |

These types and data are wrapped in a union `robj` type. And I'll explain it in another
 post.
