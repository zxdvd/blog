+++
title = "代码阅读: redis-evict"
date = "2018-02-26"
categories = ["Linux"]
tags = ["read-code", "c", "redis", "evict"]
slug = "read-code-redis-evict"
Authors = "Xudong"
+++


evict.c 是处理memory达到限制后的丢key策略和方式.

先看看一些相关的server的设置

### server
在server.h里面有不少跟lru以及evict相关的设置

    struct redisServer {
      unsigned long long maxmemory;     // 最大可用内存
      int maxmemory_samples;         // 每次随机取的key的个数
      int maxmemory_policy;        // 丢key的策略
      // 对数系数，默认是10, 越大的话, 一个key需要达到最大lfu标记所需要的被访问次数就越多
      unsigned int lfu_log_factor;
      // lfu的counter多久衰减一次, 默认是1表示一分钟
      unsigned int lfu_decay_time;
      int lazyfree_lazy_eviction;  // 是否需要lazyfree, 默认0
      long long stat_evictedkeys;  // 统计信息, 丢掉个数
    }

#### key evict设置
server.h里的一些宏定义

    #define MAXMEMORY_FLAG_LRU (1<<0)
    #define MAXMEMORY_FLAG_LFU (1<<1)
    #define MAXMEMORY_FLAG_ALLKEYS (1<<2)
    #define MAXMEMORY_FLAG_NO_SHARED_INTEGERS \
                                        (MAXMEMORY_FLAG_LRU|MAXMEMORY_FLAG_LFU)

    // 仅在设置有expire的key上执行LRU策略, 跟ALLKEYS对应
    #define MAXMEMORY_VOLATILE_LRU ((0<<8)|MAXMEMORY_FLAG_LRU)
    // expire keys 按照least frequent used策略
    #define MAXMEMORY_VOLATILE_LFU ((1<<8)|MAXMEMORY_FLAG_LFU)
    // 在expire的key空间里丢key并且TTL短的优先
    #define MAXMEMORY_VOLATILE_TTL (2<<8)
    // 在expire的key空间随机丢key
    #define MAXMEMORY_VOLATILE_RANDOM (3<<8)
    // 所有key上执行LRU策略
    #define MAXMEMORY_ALLKEYS_LRU ((4<<8)|MAXMEMORY_FLAG_LRU|MAXMEMORY_FLAG_ALLKEYS)
    #define MAXMEMORY_ALLKEYS_LFU ((5<<8)|MAXMEMORY_FLAG_LFU|MAXMEMORY_FLAG_ALLKEYS)
    // 所有key空间随机丢key
    #define MAXMEMORY_ALLKEYS_RANDOM ((6<<8)|MAXMEMORY_FLAG_ALLKEYS)
    #define MAXMEMORY_NO_EVICTION (7<<8)      // 不丢key

    // 这是默认的设置也就是不丢key, 如果add相关命令客户端会收到报错信息
    #define CONFIG_DEFAULT_MAXMEMORY_POLICY MAXMEMORY_NO_EVICTION


#### getLRUClock
获取当前的LRUClock表示的时间.
mstime是在server.c里定义的函数，获取当时时间的unix time形势(单位是毫秒),
LRU_CLOCK_RESOLUTION这个宏值是1000, 这里相当于获取到秒了, LRU_CLOCK_MAX是1<<24-1, 跟它与一下就确定返回值不会超过24bit.
因为redisObject这个struct里的lru就是24bits. 而且哪怕不需要lru策略这24bits无论如何也省不了.

   unsigned int getLRUClock(void) {
       return (mstime()/LRU_CLOCK_RESOLUTION) & LRU_CLOCK_MAX;
   }

#### LFULogIncr LFU的counter增加算法
这个函数在db.c的lookupKey里被调用的(设置为LFU的时候).
这个算法具体没有研究，但是可以看出来，随着counter越大，p就越小，导致counter++的概率就越小.
这里也能看出server.lfu_log_factor这个系数的作用，是成倍缩小了counter++的概率.
按照redis官方估计, 默认值10的时候大约要10万次hits才能使counter到142，百万次到255，如果改成100的话，百万次才到143，千万次hits到255.

    uint8_t LFULogIncr(uint8_t counter) {
        if (counter == 255) return 255;
        double r = (double)rand()/RAND_MAX;
        double baseval = counter - LFU_INIT_VAL;
        if (baseval < 0) baseval = 0;
        double p = 1.0/(baseval*server.lfu_log_factor+1);
        if (r < p) counter++;
        return counter;
    }

#### unsigned long LFUDecrAndReturn(robj *o) 减小的策略
可以发现大于四倍的LFU_INIT_VAL的时候是倍减的，小于两倍是每次减1，中间的有特别处理.

    unsigned long ldt = o->lru >> 8;
    unsigned long counter = o->lru & 255;
    if (LFUTimeElapsed(ldt) >= server.lfu_decay_time && counter) {
        if (counter > LFU_INIT_VAL*2) {
            counter /= 2;
            if (counter < LFU_INIT_VAL*2) counter = LFU_INIT_VAL*2;
        } else {
            counter--;
        }
        o->lru = (LFUGetTimeInMinutes()<<8) | counter;
    }
    return counter;



#### o->lru的初始化
在createObject(object.c)里有这么一段, 也就是根据server的最大内存策略是LRU还是LFU有不同的初始化，
这里虽然叫o.lru实际上是FRU和LFU共用的字段. 可以发现哪怕配置了NO_EVICTION, 一样会设置lru.
在LRU情况下很简单，就是设置成当前的LRU_CLOCK时间.
对于LFU的情况，要复杂很多，o->lru被分成了两部分，低8位是counter, 但是不是简单的加一或者减一，
它一开始有初始化值LFU_INIT_VAL(如果初始化是0的话很容易导致新key立即被evict了),
这个值的递减策略也很奇怪，如果大于2倍的LFU_INIT_VAL就是倍减的, 少于这个是每次减1.
前16bits是精度为分钟的当前时间(只取低16bits).

    if (server.maxmemory_policy & MAXMEMORY_FLAG_LFU) {
        o->lru = (LFUGetTimeInMinutes()<<8) | LFU_INIT_VAL;
    } else {
        o->lru = LRU_CLOCK();
    }

#### void evictionPoolPopulate(int dbid, dict *sampledict, dict *keydict, struct evictionPoolEntry *pool)
从sampledict里面抽取若干个key(由server.maxmemory_samples确定，默认5个), 然后计算idle并跟当前的idle pool里的比较以确定是否加进去以及位置.
计算idle根据采用的是LRU, LFU, TTL这三种不同的策略来分别计算的, 如下

    if (server.maxmemory_policy & MAXMEMORY_FLAG_LRU) {
        idle = estimateObjectIdleTime(o);
    } else if (server.maxmemory_policy & MAXMEMORY_FLAG_LFU) {
        idle = 255-LFUDecrAndReturn(o);
    } else if (server.maxmemory_policy == MAXMEMORY_VOLATILE_TTL) {
        idle = ULLONG_MAX - (long)dictGetVal(de);
    }

可以发现对于LRU或者LFU，都需要或者该key对应的robj来计算，对于TTL策略，只需要过期时间就可以，也就是db->expires里该key对应的value就可以了, 所有这段代码前面有一段根据不同条件获取robj的代码.

#### int freeMemoryIfNeeded(void)
前面有很大一部分的处理就是为了获取需要释放的内存大小，由总的malloc得到的内存减去slaves的buffer和aof的buffer占用的大小.

后面的处理过程概括下大致如下

    while (mem_freed < mem_tofree) {
        if (policy == LRU || policy == LFU || policy == TTL) {
            LOOP server.db -> evictionPoolPopulate();
            LOOP EvictionPoolLRU -> find bestkey;
        }
        else if (policy == RANDOM) {
            bestkey = RANDOM_KEY_FROM server.db;
        }
        if (bestkey) {
            propagatExpire();
            delete key;         // sync or async
            add stat;
            notifyKeyspaceEvent;
        }
    }


### Reference:
1. [github: redis](https://github.com/antirez/redis/blob/unstable/src/evict.c)
2. [redis: lru-cache](https://redis.io/topics/lru-cache)
