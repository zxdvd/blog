+++
title = "代码阅读: redis-cluster"
date = "2018-05-12"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-cluster"
Authors = "Xudong"
+++

先看看官方的文档了解cluster的一些特点再看代码.

1. 设计到多个key的命令需要依赖于hash tag来确保这些key在同一个节点上(如果不在同一节点则无法处理)
2. cluster模式没有多个database的概念，只有db 0, 所以也没有select命令.

Q A
1. migrate 过程中如果有多key操作怎么办?

#### int clusterLoadConfig(char *filename)
配置文件的处理，比如

    <nodeid> <ip:port> <flags> <master nodeid if not master> <ping_sent> <pong_received> <configEpoch> <slot info>
    07c37dfeb235213a872192d90877d0cd55635b91 127.0.0.1:30004 slave e7d1eecce10fd6bb5eb35b9f99a514335d9ba9ca 0 1426238317239 4 connected

#### void clusterInit(void)

    server.cluster = zmalloc(sizeof(clusterState));
    server.cluster->myself = NULL;
    server.cluster->currentEpoch = 0;
    server.cluster->state = CLUSTER_FAIL;
    server.cluster->size = 1;
    server.cluster->todo_before_sleep = 0;
    server.cluster->nodes = dictCreate(&clusterNodesDictType,NULL);
    server.cluster->nodes_black_list = dictCreate(&clusterNodesBlackListDictType,NULL);
    server.cluster->failover_auth_time = 0;
    server.cluster->failover_auth_count = 0;
    server.cluster->failover_auth_rank = 0;
    server.cluster->failover_auth_epoch = 0;
    server.cluster->cant_failover_reason = CLUSTER_CANT_FAILOVER_NONE;
    server.cluster->lastVoteEpoch = 0;
    server.cluster->stats_bus_messages_sent = 0;
    server.cluster->stats_bus_messages_received = 0;
    memset(server.cluster->slots,0, sizeof(server.cluster->slots));
    clusterCloseAllSlots();

    // 锁住文件避免出现多个node使用同一个文件的问题
    if (clusterLockConfig(server.cluster_configfile) == C_ERR)
        exit(1);

    if (clusterLoadConfig(server.cluster_configfile) == C_ERR) {
        myself = server.cluster->myself =
            createClusterNode(NULL,CLUSTER_NODE_MYSELF|CLUSTER_NODE_MASTER);
        serverLog(LL_NOTICE,"No cluster configuration found, I'm %.40s",
            myself->name);
        clusterAddNode(myself);
        saveconf = 1;
    }
    if (saveconf) clusterSaveConfigOrDie(1);

    server.cfd_count = 0;
    if (server.port > (65535-CLUSTER_PORT_INCR)) {
        serverLog(LL_WARNING, "Redis port number too high. ");
        exit(1);
    }

    if (listenToPort(server.port+CLUSTER_PORT_INCR,
        server.cfd,&server.cfd_count) == C_ERR)
    {
        exit(1);
    } else {
        int j;
        for (j = 0; j < server.cfd_count; j++) {
            if (aeCreateFileEvent(server.el, server.cfd[j], AE_READABLE,
                clusterAcceptHandler, NULL) == AE_ERR)
                    serverPanic("Unrecoverable error creating Redis Cluster "
                                "file event.");
        }
    }

    server.cluster->slots_to_keys = zslCreate();
    myself->port = server.port;
    myself->cport = server.port+CLUSTER_PORT_INCR;
    if (server.cluster_announce_port)
        myself->port = server.cluster_announce_port;
    if (server.cluster_announce_bus_port)
        myself->cport = server.cluster_announce_bus_port;
    server.cluster->mf_end = 0;
    resetManualFailover();

#### unsigned int keyHashSlot(char *key, int keylen)
keyhash的计算方法，可以发现是找第一个{和第一个}之间的string, 该string为空以及其它情况都是计算整个key的.

    for (s = 0; s < keylen; s++)
        if (key[s] == '{') break;
    if (s == keylen) return crc16(key,keylen) & 0x3FFF;
    for (e = s+1; e < keylen; e++)
        if (key[e] == '}') break;
    if (e == keylen || e == s+1) return crc16(key,keylen) & 0x3FFF;
    // 取低14bit
    return crc16(key+s+1,e-s-1) & 0x3FFF;
}




### Reference:
1. [github: redis cluster](https://github.com/antirez/redis/blob/unstable/src/cluster.c)
2. [redis: cluster specification](https://redis.io/topics/cluster-spec)
3. [redis: cluster nodes](https://redis.io/commands/cluster-nodes)
