+++
title = "代码阅读: redis-tstring"
date = "2018-01-06"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-tstring"
Authors = "Xudong"
+++

t_string.c这个模块就是最简单的key, value的get/set操作.

#### void setGenericCommand(client *c, int flags, robj *key, robj *val, robj *expire, int unit, robj *ok_reply, robj *abort_reply)
flags就是SET_NX, SET_XX, SET_EX, SET_PX这几种的组合.
unit是expire的单位，有秒和毫秒两种，对应SET_EX和SET_PX.
这里就是解析参数和对应的处理，设置key的value，如果有expire设置expire，通知keyspace发生了set/expire事件.

set/setnx/setex/psetex都是基于这个函数处理的.

后面还有get/getset/setrange/getrange/mget/mset/msetnx/incr/decr/incrby/decrby/append/strlen等命令的处理.

### Reference:
1. [github: redis zmalloc](https://github.com/antirez/redis/blob/unstable/src/t_string.c)
