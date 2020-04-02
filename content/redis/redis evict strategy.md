<!---
tags: redis, evict
-->

Redis is often used as a high performance cache server. It has many strategies to
 to deal with memory use out.

```c
#define MAXMEMORY_FLAG_LRU (1<<0)
#define MAXMEMORY_FLAG_LFU (1<<1)
#define MAXMEMORY_FLAG_ALLKEYS (1<<2)

#define MAXMEMORY_VOLATILE_LRU ((0<<8)|MAXMEMORY_FLAG_LRU)
#define MAXMEMORY_VOLATILE_LFU ((1<<8)|MAXMEMORY_FLAG_LFU)
#define MAXMEMORY_VOLATILE_TTL (2<<8)
#define MAXMEMORY_VOLATILE_RANDOM (3<<8)
#define MAXMEMORY_ALLKEYS_LRU ((4<<8)|MAXMEMORY_FLAG_LRU|MAXMEMORY_FLAG_ALLKEYS)
#define MAXMEMORY_ALLKEYS_LFU ((5<<8)|MAXMEMORY_FLAG_LFU|MAXMEMORY_FLAG_ALLKEYS)
#define MAXMEMORY_ALLKEYS_RANDOM ((6<<8)|MAXMEMORY_FLAG_ALLKEYS)
#define MAXMEMORY_NO_EVICTION (7<<8)
```

I'll explain these 8 strategies using following tables.

| strategy        | effected keys   | evict method |
| --------------- | --------------- | ------------ |
| VOLATILE_LRU    | expired keys    | LRU          |
| VOLATILE_LFU    | expired keys    | LFU          |
| VOLATILE_TTL    | expired keys    | TTL          |
| VOLATILE_RANDOM | expired keys    | RANDOM       |
| ALLKEYS_LRU     | all keys        | LRU          |
| ALLKEYS_LFU     | all keys        | LFU          |
| ALLKEYS_RANDOM  | all keys        | RANDOM       |
| NO_EVICTION     | no key effected | no evict     |

The `NO_EVICTION` means that it won't evict any key if it reaches max memory. However,
 then you can't write any new keys into it.

If you want to choose a evict strategy, you need to consider about two things:

- should I evict only expired keys or all keys
- choose LRU, LFU, TTL or random choose keys to evict


If you just want a simple cache server, no need to set expire for each key, just set
 a max memory and evict strategy like following and you'll save lots of memory (need
 not to use expire dict).

```
maxmemory 500mb
maxmemory-policy allkeys-lru
```

### References:
- [official doc: redis config](https://redis.io/topics/config)
