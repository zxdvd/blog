+++
title = "代码阅读: redis-util"
date = "2018-01-13"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-util"
Authors = "Xudong"
+++

这里都是一些字符串处理的helper函数.

#### int stringmatchlen(const char *pattern, int patternLen, const char *string, int stringLen, int nocase)
这一个是glob模式的字符串匹配函数, 支持*, ?, [a-z]这几种的处理.

#### uint32_t digits10(uint64_t v)
获取v的位数，这里为了提高性能做了些特别处理.




### Reference:
1. [github: redis anet](https://github.com/antirez/redis/blob/unstable/src/util.c)
