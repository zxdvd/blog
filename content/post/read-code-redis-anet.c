+++
title = "代码阅读: redis-anet"
date = "2018-01-07"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-anet"
Authors = "Xudong"
+++

anet.*是处理socket相关的模块，如果想写一个健壮的网络程序，可以好好看看这个模块.

总体看，这里有这么些操作
1. 设置socket的blocking和non-blocking
2. 设置keepalive, 避免长时间idle的连接被防火墙关闭，以及及时清理死连接
3. 设置tcp的nodelay，tcp是字节流，设置这个避免系统合并多个write/read,确保用户的send立即发送.
4. 设置buffer大小.
5. 设置socket的超时时间
6. 设置socket的address reuse(多个程序共用同一个端口)

然后就是对socket, bind, listen, accpet, connect的封装. 还有对read, write的封装，在非阻塞环境下模拟阻塞的效果，这里跟rio.h的rioRead, rioWrite非常类似.

#### int anetSetBlock(char *err, int fd, int non_block)

    if ((flags = fcntl(fd, F_GETFL)) == -1) {
        anetSetError(err, "fcntl(F_GETFL): %s", strerror(errno));
        return ANET_ERR;
    }

    if (non_block)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) == -1) {
        anetSetError(err, "fcntl(F_SETFL,O_NONBLOCK): %s", strerror(errno));
        return ANET_ERR;
    }
获取fd的flags，然后设置blocking/non-blocking, 然后fcntl设置一下.

#### int anetKeepAlive(char *err, int fd, int interval)
这个函数名就很明确了，设置socket的keepalive特性，但是看代码发现没有相信中那么简单，一共设置了这些东西

    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &1, sizeof(1))
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &interval, sizeof(interval))
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval/3, sizeof(interval/3)
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &3, sizeof(3))
后面三个是linux自己的设置，`man tcp`里说了，这个不具有portable性质，之所以需要后面三个是因为linux默认的设置太长了.
通过后面几个设置可以更灵活的设置探测时间(redis默认是300s)
1. TCP_KEEPIDLE: 这就是更改探测时间(默认7200s)
2. TCP_KEEPINTVL: 如果探测包没有收到响应，tcp不会立即close，而是继续重试，这个是发送重试探测包的时间间隔(默认是75s)
3. TCP_KEEPCNT: 这个是重试的次数(默认是9次)

#### tcp 设置 NO_DELAY

    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val))
这个设置可以禁用TCP的nagle算法.
nagle算法是为了避免传输很多的很小的包，当程序write的时候，kernel不会真正的立即发送
这个包，而是多等一会儿，试图跟后面的write合并，多个包一起发送(tcp是字节流，跟udp不一样，理论上合并没什么问题).
禁用nagle算法，就是立即发送, 所谓的NO_DELAY.

#### 设置socket读写的buffer大小

    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffsize, sizeof(buffsize))
    SO_RCVBUF是设置接受的buffer大小.

#### 设置socket的读写timeout

    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv))
这个是设置发送(write)时的timeout, SO_RCVTIMEO是设置接受的.

#### 设置socket的SO_REUSEADDR

    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))
设置这个后，有多个作用
1. 如果一个程序bind在`*:3000`上，另一个可以bind到一个具体的ip上如`127.0.0.1:3000`
2. 在程序crash的时候(或者其它没有正常关闭socket), 可能出现tcp还在TIME_WAIT阶段，这种时间导致重启会提示EADDREUSE错误，
设置这个可以让程序快速重启，看redis这里的注释应该更多的是为了这个原因(因为redis benchmark会短时间大量的新建/关闭socket)
3. 还有一些其它的作用以及副作用，可以看看相关书籍

#### static int anetCreateSocket(char *err, int domain)

    if ((s = socket(domain, SOCK_STREAM, 0)) == -1) { err_handle }
    if (anetSetReuseAddr(err,s) == ANET_ERR) { err_handle }
主要就是这两个了，创建socket和设置REUSEADDR

#### static int anetTcpGenericConnect(char *err, char *addr, int port, char *source_addr, int flags)
这里是对connect系统调用的封装. 支持一些flags.
我比较奇怪这里面的socket创建没有调用anetCreateSocket这个函数.
后面的anetTcpConnect, anetTcpNonBlockBindConnect都是基于这个处理的.

#### int anetUnixGenericConnect(char *err, char *path, int flags)
这个是对unix domain的connect系统调用的封装.
可以看到这里socket创建是通过anetCreateSocket函数来创建的.

#### int anetRead(int fd, char *buf, int count)
这里是通过多次调用read系统调用来实现读取count字节的数据，注意函数返回的是实际读取的字节数(读到末尾的时候可能不足count了，这时候就返回了), 返回值-1表示出错了.
`anetWrite`也是同理.

#### static int anetListen(char *err, int s, struct socketaddr *sa, socketlen_t len, int backlog)
这里是对listen系统调用的封装，先bind然后listen.

#### static int _anetTcpServer(char *err, char *bindaddr, int af, int backlog)
创建tcp socket服务端.

#### int anetUnixServer(char *err, char *path, mode_t perm, int backlog)
创建unix domain socket的服务端.

#### static int anetGenericAccept(char *err, int s, struct sockaddr *sa, socklen_t *len)
这个是对accept的封装，对EINTR错误(signal中断导致的)进行了特别处理.

#### int anetTcpAccept(char *err, int s, char *ip, size_t ip_len, int *port)
调用anetGenericAccept，然后把对端地址转换成字符串的ip和int类型的port. (ipv4和ipv6分别处理)

#### int anetPeerToString(int fd, char *ip, size_t ip_len, int *port)
通过getpeername系统调用，获取对端的地址，转换成字符串ip和int的port.(ipv4, ipv6, unix domain socket分别处理)


### Reference:
1. [github: redis anet](https://github.com/antirez/redis/blob/unstable/src/anet.c)
2. [cnblogs: tcp keepalive选项](http://www.cnblogs.com/cobbliu/p/4655542.html)
