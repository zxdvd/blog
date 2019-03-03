+++
title = "代码阅读: redis-networking"
date = "2019-02-24"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-networking"
Authors = "Xudong"
+++

主要是client的网络交互等.

#### void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask)
server初始化时，所有的listening sockets都会放入eventloop里，这里是对应的callback处理函数.
这里会尝试最多accept 1000次(MAX_ACCEPTS_PER_CALL定义), 每个accpet的connection交给acceptCommenHandler处理.

#### static void acceptCommonHandler(int fd, int flags, char *ip)
这个是client连接的实际处理函数. 主要是各种检查，然后最重要的就是调用了createClient函数.
createClient之后会检查当前client个数是否超过了，超过就发送错误提醒.
另外如果没有设置密码并且listen在所有端口上(0.0.0.0, ::1)也会发送一个错误提示(不安全).

#### client *createClient(int fd)
创建client，client有两种，一种是真实的(fd!=-1), 这种需要设置fd非阻塞，tcpnodelay,keepalive以及将这个client的fd添加到eventloop里,
并设置回调函数为readQueryFromClient.

另外一种是虚拟的client，给lua脚本之类的使用，因为lua里也会执行一些命令，然后这些命令的方法都是以client作为
参数封装好了，弄一个虚拟的client可以方便lua里面使用这些命令.

#### void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask)
准备好buffer，然后read，最后调用processInputBufferAndReplicate(c)来处理.


#### void processInputBufferAndReplicate(client *c)
这里有一些对master client的处理(用来给slave同步数据的), 然后就是正常处理.
    if (!(c->flags & CLIENT_MASTER)) {
        processInputBuffer(c);
    }

#### void processInputBuffer(client *c)
根据第一个字符判断出单个命令还是多行命令的处理. 然后调不同的方法处理参数，参数处理完成后调用processCommand(server.c)来执行命令.

        if (!c->reqtype) {
            if (c->querybuf[c->qb_pos] == '*') {
                c->reqtype = PROTO_REQ_MULTIBULK;
            } else {
                c->reqtype = PROTO_REQ_INLINE;
            }
        }
        ...
        if (processCommand(c) == C_OK) {...}

#### int processInlineBuffer(client *c)
读取一行，parse出argv.

#### int processMultibulkBuffer(client *c)
多行命令的处理.




### Reference:
1. [github: redis networking](https://github.com/antirez/redis/blob/unstable/src/networking.c)
