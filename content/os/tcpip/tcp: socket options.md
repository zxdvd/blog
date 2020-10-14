<!---
tags: linux, network, tcp
-->

## tcp socket options

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
- [blog: SO_LINER option](http://alas.matf.bg.ac.rs/manuals/lspe/snode=105.html)
