+++
title = "代码阅读: redis-quicklist"
date = "2018-01-14"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-quicklist"
Authors = "Xudong"
+++


对于__quicklistCompress的逻辑还需要继续理解，对于fill需要继续研究.

quicklist又是一个双向链表，它的payload是ziplist(一个字符串表达的双向链表), 所以可以把它看作一个二维的链表.

### quicklist.h

    typedef struct quicklistNode {
        struct quicklistNode *prev;
        struct quicklistNode *next;
        unsigned char *zl;
        unsigned int sz;             // ziplist的字节长度大小
        unsigned int count : 16;     // ziplist里的元素个数
        unsigned int encoding : 2;   // raw=1表示原始的，LZF=2表示lzf压缩的
        unsigned int container : 2;  /* NONE==1 or ZIPLIST==2 */
        // recompress为true表示node被临时解压来使用, 可以压缩
        unsigned int recompress : 1;
        unsigned int attempted_compress : 1; /* node can't compress; too small */
        unsigned int extra : 10; // 目前作为padding，留作以后使用
    } quicklistNode;

    typedef struct quicklistLZF {
        unsigned int sz; // 压缩后的长度，也就是compressed的长度
        char compressed[];
    } quicklistLZF;
这是ziplist采用LZF压缩后的结果，前面4字节的sz存的是压缩后的长度(原始长度在quicklistNode->sz里), 压缩后，quicklistNode->zl指向这个结构.

    typedef struct quicklist {
        quicklistNode *head;
        quicklistNode *tail;
        unsigned long count;   // 链表所存元素总个数(所有ziplist里存的元素个数之和)
        unsigned int len;      // 链表node个数
        int fill : 16;              /* fill factor for individual nodes */
        unsigned int compress : 16; /* depth of end nodes not to compress;0=off */
    } quicklist;

    typedef struct quicklistIter {
        const quicklist *quicklist;     // 所遍历的链表
        quicklistNode *current;         // 当前处理的node
        unsigned char *zi;
        long offset;         // 在当前ziplist里的偏移
        int direction;       // 遍历的方向(从头到尾，从尾到头)
    } quicklistIter;

    // 用来存ziplist里取出来的一个元素, 包含上层各节点信息
    typedef struct quicklistEntry {
        const quicklist *quicklist;
        quicklistNode *node;
        unsigned char *zi;      // 元素起始指针
        unsigned char *value;   // 如果是字符串, 起始指针
        long long longval;      // 如果是数字存这里
        unsigned int sz;        // 如果是字符串, 存长度
        int offset;             // 在所在节点的index偏移
    } quicklistEntry;


### quicklist.c

#### int __quicklistCompressNode(quicklistNode *node)
尝试用lzf压缩node里的ziplist, 如果本身太小或者压缩减少的字节比较少，都会取消压缩.
压缩后node->zl指向压缩后的地址, encoding和recompress都会改变.



#### void __quicklistInsertNode(quicklist *quicklist, quicklistNode *old_node, quicklistNode *new_node, int after)
双向链表添加新的节点，根据after的值决定在old_node前面加还是后面加


#### int quicklistPushHead(quicklist *quicklist, void *value, size_t sz)
头部插入.

#### int quicklistPushTail(quicklist *quicklist, void *value, size_t sz)
从尾部插入一个新元素.


#### quicklist *quicklistAppendValuesFromZiplist(quicklist *quicklist, unsigned char *zl)
这个作用就是从ziplist里一个个的遍历元素，然后一个个插入ql，然后释放ziplist.
主要是被下面这个函数调用.

    quicklist *quicklistCreateFromZiplist(int fill, int compress,
                                          unsigned char *zl) {
        return quicklistAppendValuesFromZiplist(quicklistNew(fill, compress), zl);
    }

这里可能会奇怪这里为啥不直接把ziplist挂在ql上. 这个主要是用来从rdb文件恢复，将rdb里的大ziplist拆成多个小的ziplist，便于处理.


#### void __quicklistDelNode(quicklist *quicklist, quicklistNode *node)
双向链表删除一个节点.


#### void quicklistDelEntry(quicklistIter *iter, quicklistEntry *entry)
删除迭代位置的元素并更新iter(主要是处理所在node被删除的情况).


#### int quicklistReplaceAtIndex(quicklist *quicklist, long index, void *data, int sz)
根据index找到ql里的entry，然后删除旧的插入新的.


#### quicklistNode *_quicklistZiplistMerge(quicklist *quicklist, quicklistNode *a, quicklistNode *b)
合并ql里的a, b两个node, 并释放资源.


#### void _quicklistMergeNodes(quicklist *quicklist, quicklistNode *center)
合并以center为中心的多个nodes(前后各两个), 尽力而为的合并.
先尝试合并center->prev和center->prev->prev, 然后是center->next和center->next->next.
然后尝试将center跟上面合并后的两个新node继续合并.


#### quicklistNode *_quicklistSplitNode(quicklistNode *node, int offset, int after)
将节点从某个位置开始一分为二.
初始化一个新节点，将原节点的ziplist深拷贝到新节点，然后这两个节点一个删除offset前面的部分，一个删除offset后面的部分.


#### void _quicklistInsert(quicklist *quicklist, quicklistEntry *entry, void *value, const size_t sz, int after)
在entry前面或者后面(根据after的值确定前后)插入一个新的元素.
如果entry->node不存在, 直接新建一个node以及ziplist然后把这个node加入到ql里就可以.

entry->node存在的话有以下情况要处理
    *. 当前node没有满而且要插入的点也在这个node
    *. 当前节点满，在元素后面插并且后面的节点没有满
    *. 当前节点满，在前面插并且前一个节点没有满
    *. 当前节点满，并且要插入的前一个或者后一个节点也满了, 这种情况直接新建一个节点
    *. 当前节点满，并且需要在中间插，这个时候需要调用_quicklistSplitNode来将当前节点分裂


#### int quicklistDelRange(quicklist *quicklist, const long start, const long count)
从某个位置开始删除连续的count个元素.
需要处理各种正序负序以及下一个node或者前一个node的问题，逻辑并不复杂，但是细节很多.


#### quicklistIter *quicklistGetIterator(const quicklist *quicklist, int direction)
初始化一个iterator


#### quicklistIter *quicklistGetIteratorAtIdx(const quicklist *quicklist, const int direction, const long long idx)
调用quicklistIndex来获取第idx个元素，通过得到的entry来构建一个iterator的起始位置.


#### int quicklistNext(quicklistIter *iter, quicklistEntry *entry)
根据iter这个ql迭代指针的配置，获取后一个元素并放入entry.
这个可以用来对整个ql进行遍历.
这里需要注意的是处理iter当前指向某个node的最后一个元素了，需要跳到下一个node的处理,
需要将当前node解压的ziplist有压缩下, 而且要考虑从头部遍历跟尾部遍历的情况.

这里面ziplist获取下一个元素位置的处理可以拿出来看看，有点动态函数的感觉.
定义一个函数指针，初始化为NULL, 通过判断来确定它指向ziplistNext还是ziplistPrev, 然后后面调用这个函数指针.

    unsigned char *(*nextFn)(unsigned char *, unsigned char *) = NULL;
    if (iter->direction == AL_START_HEAD) {
        nextFn = ziplistNext;
    } else if (iter->direction == AL_START_TAIL) {
        nextFn = ziplistPrev;
    }
    iter->zi = nextFn(iter->current->zl, iter->zi);


#### quicklist *quicklistDup(quicklist *orig)
完整复制一个ql, 是深拷贝而不是浅拷贝.


#### int quicklistIndex(const quicklist *quicklist, const long long idx, quicklistEntry *entry)
从ql中找第idx个元素，构建一个quicklistEntry指针作为参数返回.
idx0表示第一个，1是第二个，-1是倒数第一个，-2是倒数第二个.
主要就是从头或者尾一个个node遍历，找到所在node然后解压从ziplist里面获取数据.
entry这个结构并不是简单存元素值，还存了ql，所在node, 元素起始指针,
entry->offset存的是在所在node里的偏移, entry->value, entry->sz, entry->longval存的是返回值(字符串或数字)


#### void quicklistRotate(quicklist *quicklist)
将ql的最后一个元素(也就是尾部node的ziplist里的最后一个)pop出来然后插入到最前面.


    // 调用ziplist的函数获取最后一个元素的起始位置
    unsigned char *p = ziplistIndex(quicklist->tail->zl, -1);
    // 取值(str or int)
    ziplistGet(p, &value, &sz, &longval);
    quicklistPushHead(quicklist, value, sz);
    quicklistDelIndex(quicklist, quicklist->tail, &p);


#### int quicklistPopCustom(quicklist *quicklist, int where, unsigned char **data, unsigned int *sz, long long *sval,
void*(*saver)(unsigned char *data, unsigned int sz))
先说参数吧，where是确定从头部pop还是尾部pop; char **data是用来接受pop出来的字符串, sz是pop出来的字符串的长度,
如果pop出来的是数字，前面两个都没用，数字放在long long *sval里返回, saver是一个函数指针，可以传入自定义
函数用来复制pop出来的字符串(数字就不需要)

总的逻辑就是如果从头部pop，取ql的第一个node的ziplist的第一个元素，
如果是尾部pop取ql链表的尾部的ziplist的最后一个元素.

    int pos = (where == QUICKLIST_HEAD) ? 0 : -1;
    quicklistNode *node;
    if (where == QUICKLIST_HEAD && quicklist->head) {
        node = quicklist->head;
    } else if (where == QUICKLIST_TAIL && quicklist->tail) {
        node = quicklist->tail;
    } else {
        return 0;
    }

    p = ziplistIndex(node->zl, pos);
    if (ziplistGet(p, &vstr, &vlen, &vlong)) {
        if (vstr) {
            // ziplist里返回字符串的处理
        } else {
            // ziplist里返回数字的处理
        }
        // 删除
        quicklistDelIndex(quicklist, node, &p);


#### void *_quicklistSaver(unsigned char *data, unsigned int sz)
复制data指向的数据，长度是sz字节.


#### int quicklistPop(quicklist *quicklist, int where, unsigned char **data, unsigned int *sz, long long *slong)
从ql中删除数据，被删除的数据copy出来并通过参数返回.
返回0表示没有找到，返回1是找到并删除成功.


#### void quicklistPush(quicklist *quicklist, void *value, const size_t sz, int where)
根据where是HEAD还是TAIL来确定调quicklistPushHead还是quicklistPushTail.



### Reference:
1. [github: redis quicklist](https://github.com/antirez/redis/blob/unstable/src/quicklist.c)
