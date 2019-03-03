+++
title = "代码阅读: redis-dict"
date = "2017-12-26"
categories = ["Linux"]
tags = ["read-code", "c", "redis", "dict"]
slug = "read-code-redis-dict"
Authors = "Xudong"
+++


dict.*这里面很久之前学习hash表的时候就研究过，还曾经把murmurhash用python抄了一篇(实现bloomfilter时候弄的).
这里算是redis比较核心的地方了，所有的key, value都存这dict里面.

### dict.h
dict.h里面定义了很多重要的结构，需要好好看看.

        typedef struct dictEntry {
            void *key;
            union {
                void *val;
                uint64_t u64;
                int64_t s64;
                double d;
            } v;
            struct dictEntry *next;
        } dictEntry;
这是dict里面最基本的一个key, value的结构，value是一个union v, 有指针, uint, int, double几种类型.

这里还有一个`struct dictEntry *next;`, 是因为redis的dict是链表法实现的hash，
hash值一样的多个键值对以链表形式连接起来.

        typedef struct dictType {
            unsigned int (*hashFunction)(const void *key);
            void *(*keyDup)(void *privdata, const void *key);
            void *(*valDup)(void *privdata, const void *obj);
            int (*keyCompare)(void *privdata, const void *key1, const void *key2);
            void (*keyDestructor)(void *privdata, void *key);
            void (*valDestructor)(void *privdata, void *obj);
        } dictType;

dictType里面定义了一组函数指针，看变量名分别是hash函数，
key, value复制，比较以及销毁的函数.

        typedef struct dictht {
            dictEntry **table;
            unsigned long size;
            unsigned long sizemask;
            unsigned long used;
        } dictht;

这里就是hash表的结构了，`dictEntry **table`如果改成下面这样应该很好理解一点

        typedef struct dictEntry *entry;
        entry *table;

table实际上是`*dictEntry`的指针, 可以理解成一个数组，每个元素是指向dictEntry的指针.

size是hash表的容量，used就是当前的使用量.

        typedef struct dict {
            dictType *type;
            void *privdata;
            dictht ht[2];
            long rehashidx; /* rehashing not in progress if rehashidx == -1 */
            unsigned long iterators; /* number of iterators currently running */
        } dict;

这里定义的dict是redis真正使用的dict，dictType里是一组函数指针.
ht[2]是两个之前定义的dictht结构，为什么需要两个呢？
想想hash表随着key增多，会导致很多key的hash值一样，挤到同一个槽的链表里. 导致操作复杂度上升，这个需要就需要扩容了.
比如一开始是1024个槽位，要扩容到2048个，key都需要重新hash换槽，对于redis来说，条件不允许进行阻塞式的扩容，
所有只能同时使用两个hash表采用愚公移山式的慢慢扩容了.
rehashidx记录扩容时，当前挪到那个槽位了，-1表示当前没有在扩容.

后面定义了很多对key, value进行get, set, free, compare等的宏，就不说了，都是很简单的宏.


### dict.c
`dict_force_resize_ratio=5`, 这个是指一个size为8的hash表，最多允许存40个key, value对，
再多redis就要强制进行resize了.

#### hash function
这里面定义了`dictIntHashFunction`, `dictGenHashFunction`(murmurhash2), `dictGenCaseHashFunction`(大小写不敏感)这3种hash函数.
redis之前用的是murmurhash，在2017年二月换成了siphash(为了抵御hashDOS攻击, 我个人觉得不是什么严重问题).

#### dictCreate, _dictInit
dict的创建和初始化函数

#### int dictExpand(dict *d, unsigned long size)
这个是dict扩容的函数, size是扩容后的槽位大小

        unsigned long realsize = _dictNextPower(size);
这里是因为要保持dict的槽位是2^n，找一个比size大的2的幂数.

然后后面有一些判断，比如不能扩容一个正在扩容当中的dict.

        n.table = zcalloc(realsize*sizeof(dictEntry*));
        d->ht[1] = n;
        d->rehashidx = 0;
        return DICT_OK;
初始化一个包含realsize个槽位的hash表, 并赋值给`d->ht[1]`, 然后设置`d->rehashidx`为0表示要从第0个槽位开始扩容了.

到此，整个扩容的前期工作完成了，最主要的是malloc内存建立新的hash表，但是实际上没有开始任何数据的腾挪工作.

#### int dictRehash(dict *d, int n)
dictExpand是产生新的扩容后的hash表，dictRehash这个函数就是将数据从旧hash表搬运到新的hash表.

n这个参数是表示一次搬运n个，之前说过redis的容量可能达几G什么几十G，
扩容的时候只能一点点的搬.

为什么要rehash(重新计算槽位)呢?
假设一个key hash后的值是11，如果hash表空间是8的话，`11 % 8 = 3`, 它就分配到3号槽，
但是如果这个hash表size要扩容到16的话, `11 % 16 = 11`, 它就要去11号槽啦.
所有hash扩容不是简单的把旧表copy过来.

    int empty_visits = n*10;
        while(d->ht[0].table[d->rehashidx] == NULL) {
            d->rehashidx++;
            if (--empty_visits == 0) return 1;
        }
d->rehashidx是当前需要rehash的槽位编号(从0开始)
dict进行rehash的时候，hash表的有些槽位可能是空的，这个时候只需要`d->rehashidx++;`继续弄下一个就可以了，
看代码我们会发现没有`n++`，也就是这种情况不计入n的统计. 但是如果这种空的特别多，
虽然这里的操作很简单，但是也可能block主线程一小段时间,所有开始定义了`empty_visits = n*10;`，
如果这种空操作到达这个数量了，这个函数就直接返回了.

    while(n-- && d->ht[0].used != 0) {
        dictEntry *de, *nextde;

        // de指向当前需要搬运的槽位的第一个dictEntry
        de = d->ht[0].table[d->rehashidx];
        // 因为redis的hash是链表法, 所以同一个槽位可能有多个dictEntry，需要一个个搬
        while(de) {
            unsigned int h;

            nextde = de->next;
            // 重新计算hash值然后模运算获得槽位号
            h = dictHashKey(d, de->key) & d->ht[1].sizemask;
            // 将next指向前一个插进去de
            de->next = d->ht[1].table[h];
            // 把当前de设置成该槽位链表头部
            d->ht[1].table[h] = de;
            // 原hash表的容量减1，新hash表的加1
            d->ht[0].used--;
            d->ht[1].used++;
            // 指向同槽位的链表的下一个元素
            de = nextde;
        }
        // 如果该槽位的全部挪完了，把该槽位置为NULL
        d->ht[0].table[d->rehashidx] = NULL;
        // 继续处理下一个槽位
        d->rehashidx++;
    }
1. 外层的while循环处理的是hash表的一个个槽位，由于采用的是链接法，
同一个槽位可能有多个元素，第二个while循环就是处理这多个元素组成的链表，一个个的挪
2. 仔细看里面的while循环，可以发现它把每次获得的de先将next指向当前的槽位头部，
然后把自己设置为改槽位头部，这样相当于把旧的链表倒过来了，也就是旧的尾部变成新的头部了。
不过hash表对这个链本身没有顺序要求，而且这样是最高效的，一次循环就搞定了.

    if (d->ht[0].used == 0) {
        zfree(d->ht[0].table);
        d->ht[0] = d->ht[1];
        _dictReset(&d->ht[1]);
        d->rehashidx = -1;
        return 0;
    }
当旧hash表的元素约挪越少，最终就到0了，这个时候直接把就的表干掉，
把新的表设置为默认的表(`d->ht[0] = d->ht[1];`), 然后弄一个空的备用表并且设置rehashidx为-1表示扩容结束.

注意下扩容结束的返回值是0，返回1表示扩容n步后，还没有结束.

#### int dictRehashMilliseconds(dict *d, int ms)
这个函数接受一个参数，表示从当前时间算起，进行多少毫秒的扩容，每次挪100个槽位，时间到了就退出.

#### dictEntry *dictAddRaw(dict *d, void *key)
这个是给dict添加一个新key，如果这个key存在就返回NULL. 如果dict正在进行扩容，就插到扩容的新hash表(ht[1])的头部, 否则就是默认的ht[0].

#### int dictAdd(dict *d, void *key, void *val)
调用dictAddRaw插入key，然后设置val.

#### int dictReplace(dict *d, void *key, void *val)
这个是key, value的upsert操作，先尝试dictAdd，如果成功了就返回1表示是insert操作.
如果失败了，说明key存在，查找该key对应的dictEntry，然后先disctSetVal设置新的val再dictFreeVal free掉旧的val.
注意这里有大段注释特意说明必须要先set再free，因为有可能更新的value跟现存的value是同一个指针。
那么同一个指针先set再free岂不是还是free掉了？
并不是的，如果后面仔细看dictCreate定义时候的dictType里的destructor的实现，
可以发现它是采用的引用计数的方式，而不是直接free，这样的话多个key的value
可以共用同一个指针，而不担心一个free掉了别的key也都被free了.

既然是引用计数，那就很好理解为什么要先set再free了，如果先free导致引用计数为0了，这个指针可能就真的被free了，
再set就没用了; 先set再free就不会有这样的情况.

#### dictEntry *dictReplaceRaw(dict *d, void *key)
这个是dictAddRaw的upsert实现.

#### static int dictGenericDelete(dict *d, const void *key, int nofree)
这个是从dict中删除一个key以及对应的value，从ht[0]和ht[1]里面找对应的槽位，
然后从该槽位的链表中删除. 这个函数接受一个nofree的参数来确定是否
free key和value. 由此派生出dictDeleteNoFree和dictDelete两个函数.

#### int _dictClear(dict *d, dictht *ht, void(callback)(void *d))
这个清空dict的某个ht，并且每清空65536个槽位的时候调一次callback函数(外部传进来的).

#### dictEntry *dictFind(dict *d, const void *key)
从dict的2个ht里查找某个key对应的dictEntry.

#### static int _dictExpandIfNeeded(dict *d)
这个是检查一些条件然后确定是否要扩容，以及expand到多少
1. 如果正在扩容，直接返回
2. size=0的扩容到定义的`DICT_HT_INITIAL_SIZE`，也就是4
3. 如果静态变量dict_can_resize是true，并且装载比例达到1就扩容一倍
4. 如果dict_can_resize是false，需要装载比例超过dict_force_resize_ratio(默认是5)才扩容一倍

#### static int _dictKeyIndex(dict *d, const void *key)
找到一个key对应的槽位，如果该key存在了返回-1

### Reference:
1. [github: redis dict](https://github.com/antirez/redis/blob/unstable/src/dict.c)
