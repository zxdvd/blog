+++
title = "代码阅读: redis-pubsub"
date = "2018-02-24"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-pubsub"
Authors = "Xudong"
+++

这里是对pubsub相关的命令subscribe, unsubscribe, psubscribe, punsubscribe, publish, pubsub的处理.
关于expire keys的处理，也就是主要是db->expires.

先介绍下相关的数据结构

    // channel是用dict来存储的，因为channel名字是明确的可以作为key
    // 从后面的代码里可以看到channel对应的value是clients组成的list
    server.pubsub_channels = dictCreate(&keylistDictType,NULL);
    // pattern因为有*?等不是明确的，就用list来存，查的时候需要一个个遍历
    server.pubsub_patterns = listCreate();
    listSetFreeMethod(server.pubsub_patterns,freePubsubPattern);
    listSetMatchMethod(server.pubsub_patterns,listMatchPubsubPattern);

    typedef struct client {
        dict *pubsub_channels;  // client加入的channel
        list *pubsub_patterns;  // client订阅的pattern
    }

    // 一个client和一个pattern object组成一个pubsubPattern
    typedef struct pubsubPattern {
        client *client;
        robj *pattern;
    } pubsubPattern;

#### freePubsubPattern listMatchPubsubPattern
这两个函数是给server.pubsub_patterns用的，redis里面的双向链表(adlist)里的free, dup, match是函数指针，由外部设置，这里是设置本模块的free和match,
match用来比较两个pubsubPattern是否一样, 这里认为同一个client并且pattern的字符串一样就是一样的.

#### int clientSubscriptionsCount(client *c)
返回client订阅的channel和pattern总数，就是dictSize + listLength.

#### int pubsubSubscribeChannel(client *c, robj *channel)
处理一个client订阅一个channel

    // 首先加入到client自己的pubsub_channels里面去
    // channel为key, value是NULL, 其实就是一个set数据结构
    if (dictAdd(c->pubsub_channels,channel,NULL) == DICT_OK) {
        retval = 1;
        // channel的内存管理采用引用计数的方式
        incrRefCount(channel);
        // 从server的pubsub_channels里面找这个channel
        de = dictFind(server.pubsub_channels,channel);
        // 没找到就需要创建一个键值对，key是channel，value是clients组成的list
        if (de == NULL) {
            clients = listCreate();
            dictAdd(server.pubsub_channels,channel,clients);
            incrRefCount(channel);
        } else {
            clients = dictGetVal(de);
        }
        listAddNodeTail(clients,c);
    }
    /* Notify the client */
    addReply(c,shared.mbulkhdr[3]);
    addReply(c,shared.subscribebulk);
    addReplyBulk(c,channel);
    addReplyLongLong(c,clientSubscriptionsCount(c));


#### int pubsubUnsubscribeChannel(client *c, robj *channel, int notify)
跟上面的操作相反，首先从c->pubsub_channels里删除，然后从server.pubsub_channels里找到该channel对应的list, 从该list里面删除当前客户端,
删除后list为空的话就把这个channel从dict里面删除.

#### int pubsubSubscribePattern(client *c, robj *pattern)
跟channel的处理基本类似，添加到客户端和服务器的pubsub_patterns里面去.
取消订阅也是类似.

#### int pubsubUnsubscribeAllChannels(client *c, int notify)
遍历c->pubsub_channels, 挨个调用pubsubUnsubscribeChannel来删除.

#### int pubsubPublishMessage(robj *channel, robj *message)
先在server.pubsub_channels里面找channel对应的客户端list，如果有则遍历list一个个发送.
然后处理pattern的情况，这里主要是调用stringmatchlen这个glob字符串比较函数，一个个的进行pattern匹配，匹配上的就发送.

### Reference:
1. [github: redis pubsub](https://github.com/antirez/redis/blob/unstable/src/pubsub.c)
