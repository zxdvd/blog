+++
title = "代码阅读: redis-tset"
date = "2018-03-08"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-tset"
Authors = "Xudong"
+++

set相关的处理，set实际是hash表(dict)实现的, 只不过value为NULL.

#### robj *createSetObject(void)
这是创建set对象的方法，在object.c里定义的, 底层是dict, type是OBJ_LIST, encoding是OBJ_ENCODING_HT.

    dict *d = dictCreate(&setDictType,NULL);
    robj *o = createObject(OBJ_SET,d);
    o->encoding = OBJ_ENCODING_HT;

#### robj *setTypeCreate(sds value)
set的初始化，根据元素的类型，如果是int就初始化成intset, 如果不是就初始化成dict.

    if (isSdsRepresentableAsLongLong(value,NULL) == C_OK)
        return createIntsetObject();
    return createSetObject();

#### int setTypeAdd(robj *subject, sds value)
set里添加新的value, 如果set一开始就是dict，就很简单，dictAdd就可以. 如果是intset, 则看新添加的值是否是int，如果不是，需要把之前的intset转换成dict. 如果是int，插入intset之后还会检查是否超过了intset的最大长度限制，如果超过了也要转换成dict.


    if (subject->encoding == OBJ_ENCODING_HT) {
        ADD_TO_DICT
    } else if (subject->encoding == OBJ_ENCODING_INTSET) {
        if (isSdsRepresentableAsLongLong(value,&llval) == C_OK) {
            uint8_t success = 0;
            subject->ptr = intsetAdd(subject->ptr,llval,&success);
            // 如果intset插入成功，还需要判断是否超过了intset最大长度限制(默认512个), 超过了的话要转换成dict
            if (success) {
                if (intsetLen(subject->ptr) > server.set_max_intset_entries)
                    setTypeConvert(subject,OBJ_ENCODING_HT);
                return 1;
            }
        } else {
            // 如果前面插入一堆int, 突然来个不是int的，就需要把intset转换成dict
            setTypeConvert(subject,OBJ_ENCODING_HT);
            ADD_TO_DICT
        }
    }

#### int setTypeRemove(robj *setobj, sds value)
根据两种不同的类型分开处理remove的情况

#### int setTypeIsMember(robj *subject, sds value)
两种类型的是否存在的判断

#### int setTypeRandomElement(robj *setobj, sds *sdsele, int64_t *llele)
从set里随机返回一个元素，需要考虑两种类型的处理.

#### void setTypeConvert(robj *setobj, int enc)
将intset类型的set转换成dict实现的set.

        int64_t intele;
        dict *d = dictCreate(&setDictType,NULL);
        sds element;
        // 提前将dict扩容到需要的大小, 避免后面自动扩容
        dictExpand(d,intsetLen(setobj->ptr));

        // 迭代intset, 一个个的添加到dict里
        si = setTypeInitIterator(setobj);
        while (setTypeNext(si,&element,&intele) != -1) {
            element = sdsfromlonglong(intele);
            serverAssert(dictAdd(d,element,NULL) == DICT_OK);
        }
        setTypeReleaseIterator(si);

        // 修改encoding类型, 释放旧的intset
        setobj->encoding = OBJ_ENCODING_HT;
        zfree(setobj->ptr);
        setobj->ptr = d;

#### void saddCommand(client *c)
给set里添加一个或者多个元素

#### void sremCommand(client *c)
删除元素的处理

#### void smoveCommand(client *c)
从一个set挪到另外一个(如果没有就创建)

#### void sismemberCommand(client *c)
sismember的处理.

#### void scardCommand(client *c)
返回set的长度

#### void spopWithCountCommand(client *c)
随机pop若干个出来并且返回.
函数很长是分了3中情况处理，count >= setsize的时候就整个set全部返回，然后删除这个key就完事了.
count < setsize的时候还分删除的多还是保留的多做分别处理(不是直接比大小，而是有一个系数默认是5).

#### void srandmemberWithCountCommand(client *c)
跟spopWithCountCommand很类似，只不过不需要删除元素. 还有count可以是负数，负数的时候需要的元素个数一定得是-count, 可以有重复的.

这里也是分情况处理，首先处理负数的情况，就是一个个的random获取，知道数量达到-count, 返回的里面可能有重复的, count可以比size大(这种情况必然有重复的).

然后处理正数的, 如果count>=size, 全部返回就可以了.
如果count超过size的三分之一, 会先将set复制到一个dict里去，然后从dict里一个个的random delete知道个数为count. 如果没有超过1/3，就从set里一直random获取元素插入到dict里, 知道数量满足要求. 最后返回dict.


#### void sinterGenericCommand(client *c, robj **setkeys, unsigned long setnum, robj *dstkey)
sinter的处理, 首先把所有的key对应的sets取出来放到一个array里. 然后用qsort排序这个array, 将长度最小的放前面.

然后遍历sets里的第一个set, 也就是最短的那个, 对于每一个元素, 都去其它的set里面查下看是否存在, 然后根据dstkey是否存在分别处理.

#### void sunionDiffGenericCommand(client *c, robj **setkeys, int setnum, robj *dstkey, int op)
跟sinter的处理差不多, 先把所有的key对于的set都放到sets这个array里.
对于union的处理很简单, 一个个的遍历并且setTypeAdd就可以了.
对于diff有两种算法，一种是遍历sets[0]然后检查每一个元素是否在后面所有的set里，复杂度是len(sets[0])*len(sets).
另一种是新建一个set，将set[0]全部add进去, 然后对于其它的所有set，从这个新建的set里执行删除动作，复杂度是len(set[i]), i=range(0, N).
所有这里前面专门有一段来计算这两个复杂度的比较以确定选哪一种方式.

### Reference:
1. [github: redis tset](https://github.com/antirez/redis/blob/unstable/src/t_set.c)
