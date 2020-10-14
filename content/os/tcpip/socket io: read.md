```metadata
tags: linux, network, tcp, io
```

## socket io: read

### return value
On success, the number of bytes read is returned. Zero indicates socket is closed.
On error, -1 is returned.

### blocking and non-blocking
You can set the socket mode as blocking or non-blocking. If the socket is in blocking
 mode, the `read()` will block until there is data ready in the kernel buffer.

If the socket is in non-blocking mode, the `read()` will return -1 immediately and set
 the error code as `EWOULDBLOCK` or `EAGAIN` when there isn't any data available.

So for single thread applications, you should be very careful to blocking io since it
 may block the whole thread forever if no ready data.
