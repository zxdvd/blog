+++
title = "代码阅读: redis-skiplist"
date = "2019-02-24"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-skiplist"
Authors = "Xudong"
+++

skiplist目前在zset里面有使用(skiplist+hash跟ziplist互相转换).
redis里的ziplist默认最大level数是64, p=1/4.

因为zset需要支持一些根据下标访问的操作(redis里称为rank), 所以这个skip list里额外的用一个span来记录了同一个level里相邻元素的间隔，这样能实现一些index相关的操作.

#### zskiplistNode, zskiplist, zset in server.h
els是存的元素，score是对应的分数.

    typedef struct zskiplistNode {
        sds ele;
        double score;
        struct zskiplistNode *backward;
        // level是skip list的纵向的索引, 底层的是level[0]
        // 高层可能是空的, forward是指向右边的node
        // span是间隔元素个数, 便于index相关操作
        struct zskiplistLevel {
            struct zskiplistNode *forward;
            unsigned long span;
        } level[];
    } zskiplistNode;

    typedef struct zskiplist {
        struct zskiplistNode *header, *tail;
        unsigned long length;
        int level;
    } zskiplist;

    typedef struct zset {
        dict *dict;
        zskiplist *zsl;
    } zset;


#### zskiplistNode* zslGetElementByRank(zskiplist *zsl, unsigned long rank)
查找的路径跟查找元素一样的，只不过比较的不是score而是span，span累加得到的就是当前节点的index, 所以只需要找到累加的span等于rank就完成了.

    unsigned long traversed = 0;
    x = zsl->header;
    for (i = zsl->level-1; i >= 0; i--) {
        while (x->level[i].forward && (traversed + x->level[i].span) <= rank)
        {
            traversed += x->level[i].span;
            x = x->level[i].forward;
        }
        if (traversed == rank) {
            return x;
        }
    }

#### zskiplistNode *zslInsert(zskiplist *zsl, double score, sds ele)
update数组是用来存每一个level最右边的score满足条件的node，这样在后续一个个level插入的时候好定位. rank数组是保存每一个level最右边那个node在整个
skiplist里的当前index, 后续根据这个来更新span.

    zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x;
    unsigned int rank[ZSKIPLIST_MAXLEVEL];


### Reference:
1. [github: redis tzset](https://github.com/antirez/redis/blob/unstable/src/t_zset.c)
