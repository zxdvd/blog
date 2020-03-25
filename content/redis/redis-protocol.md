<!---
tags: redis, protocol
-->

## redis: the RESP protocol

Client and server need a protocol to communiate. Redis uses the RESP (redis serialization
 protocol). It's a text protocol that's each to read and understand. On the other side,
 it's not so efficient as binary protocol.

### data types
RESP supports following data types:

- Simple String: first byte is "+", like `+OK\r\n`
- Error: first byte is "-", like `-Error Message\r\n`
- Integer: first byte is ":", like `:0\r\n`
- Bulk String: begins with a "$" and string length, like `$6\r\nfoobar\r\n`
    + `$0\r\n\r\n` is empty string
    + `$-1\r\n` is NULL
- Array: a collection of elements, begins with a "*" and array length,
  like `*2\r\n:100\r\n$3\r\nfoo\r\n`, array composed of two elements - [100, foo].
  Array of array is also supported

### implementation
You can get a lot of `addReplyXXX` functions at `src/networking.c`.

### references
[redis.io: protocol](https://redis.io/topics/protocol)
