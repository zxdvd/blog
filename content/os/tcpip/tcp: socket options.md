<!---
tags: linux, network, tcp
-->

## tcp socket options

## Nagle's algorithm and TCP_NODELAY
Before explain TCP_NODELAY, I'll introduce a little about Nagle's algorithm.

The Nagle's algorithm comes to improve efficiency of TCP/IP by accumulating small chunks
 to large to reduce the TCP/IP header payload.

For example, when you write only one byte, the packet will have at least 40 bytes for
 TCP/IP headers (20 bytes TCP header, 20 bytes ipv4 header). The valid throughput is
 1/40. By accumulating to a large chunk, suppose 100 bytes or 300 bytes, we can improve
 throughput dramatically.

Following is the pseudo code of the algorithm:

```
// from wikipedia
if there is new data to send then
    if the window size ≥ MSS and available data is ≥ MSS then
        send complete MSS segment now
    else
        if there is unconfirmed data still in the pipe then
            enqueue data in the buffer until an acknowledge is received
        else
            send data immediately
        end if
    end if
end if
```

Now it's enabled by default in most operating systems (windows, linux). But sometimes,
 if your application prefers realtime interaction and don't care about the throughput,
 you may want to disable it. Then the `TCP_NODELAY` is used for this. If this flag
 is enabled, it will send immediately.

#### Nagle's algorithm and delayed ACK
To improve efficiency, TCP/IP also uses delayed ACK: wait a moment and send only one
 ACK for multiple received packets. The delay is less than 500ms according to the RFC.

Nagle's algorithm will send buffered data after receiving delayed ACK. It may lead to
 slow interaction in some applications and you can try to disable Nagle's algorithm.

### TCP_CORK (TCP_NOPUSH in bsd)
TCP_CORK is an enhancement of the Nagle's algorithm. It's used with framed application
 messages like HTTP, websocket.

You can enable this flag and begin to write data, for example, http headers. And it
 will be buffered. You can continue to write, for example, http body. After the whole
 frame is done, you can clear the TCP_CORK flag via `setsockopt` and then the buffered
 data is sent immediately.

There is a 200ms delay timer for TCP_CORK so that application should finish the whole
 frame in 200ms so that it can get best effect.

### the SO_LINER option
The SO_LINER option is not used so frequently. It is used to control the behavior
 of the `close()`. By default, `close()` is an async api, it will return immediately.
However, there may still have data in the kernel buffer that hadn't been sent
 successfully. Then use cannot deal with this since socket closed. This is left to
 be handled by kernel.

You can change this behavior via the SO_LINER option. It is a composed option defined
 like following

```c
// linux include/linux/socket.h
struct linger {
	int		l_onoff;	/* Linger active		*/
	int		l_linger;	/* How long to linger for	*/
};
```

The l_onoff acts as a bool value, zero means false. If it is 0 (false), then l_linger is
 ignored and default behavior implied.

If l_onoff is non-zero (true), then l_linger is timeout in seconds that `close()` will
 wait. If all data sent successfully, `close()` will return successfully. Otherwise
 you'll get `EWOULDBLOCK` if data sent failed or timeout.

And there is a special case when l_onoff is true:

    if l_linger is 0, then it will abort the connection with RST packet when close.

### references
- [man page: tcp](https://linux.die.net/man/7/tcp)
- [wikipedia: Nagle's algorithm](https://en.wikipedia.org/wiki/Nagle's_algorithm)
- [rfc1122: when to send an ACK](https://tools.ietf.org/html/rfc1122#page-96)
- [blog: SO_LINER option](http://alas.matf.bg.ac.rs/manuals/lspe/snode=105.html)
