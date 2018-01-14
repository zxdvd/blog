+++
title = "代码阅读: redis-intset"
date = "2018-01-06"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis-intset"
Authors = "Xudong"
+++

inset.*是一个int的set类型实现(元素是uniq的).
看完全部代码会发现它是这么实现的
1. 所有元素从小到大按序存在`char *`结构里.
2. 对于每个intset都有一个encoding来确定他们是int16_t还是int32_t还是int64_t, 如果有任何一个元素是int64_t那么所有的都是按照int64_t存的
3. 默认是按照int16_t存，假设存了100个，然后来了一个int64_t的，那么需要重写每个元素，把所有的都按照int64_t重写一遍

### intset.h

    typedef struct intset {
        uint32_t encoding;
        uint32_t length;
        int8_t contents[];
    } intset;
这个就是intset的结构, encoding里存的是int16_t, int32_t, int64_t的size大小，这里不需要用uint32_t这么大的类型来存，用这个应该只是为了对齐，感觉可以放在contents的首部占一字节.
length就是长度, 不是contents的字节长度，而是元素的个数
contents是一个char *指针存intset内容的.

### intset.c

#### static int64_t _intsetGetEncoded(intset *is, int pos, uint8_t enc)
这个是根据具体的encoding和pos来获取inset里的元素，这里enc是必须的，不然没法计算元素在contents里的真正偏移.

#### static void _intsetSet(intset *is, int pos, int64_t value)
这里没有检查value的encoding是否超过了is的当前encoding，因此调用这个函数之前需要检查.

#### static uint8_t intsetSearch(intset *is, int64_t value, uint32_t *pos)
这个函数查找value是否已经在is里了，如果在把下标写入pos返回，如果不在，把value应该插入的下标写入pos返回.
因为intset是拍好序的，所有可以看到这里用了二分查找.

    mid = ((unsigned int)min + (unsigned int)max) >> 1;
这里多说一点，这个是为了规避int越界的问题, 比如max=INT_MAX, min=1, (min+max)就超过int范围了，这里转换成uint就没问题了(uint范围大一倍)

#### static intset *intsetUpgradeAndAdd(intset *is, int64_t value)
这个是出现之前说的如果突然来了一个数，encoding比当前大(比如int16_t的intset来了个int32_t),
这个时候需要对整个intset重写了，首先当然是resize了，比如原来是100个两字节的int16_t, 现在需要四字节的int32_t, 大小增大一倍啦.
然后得从contents里面按照原来的encoding一个个获取元素再按照新的encoding写进去(每个元素都由两字节变四字节了).
然后还得处理value的插入问题

    int prepend = value < 0 ? 1 : 0;
    /* Set the value at the beginning or the end. */
    if (prepend)
        _intsetSet(is,0,value);
    else
        _intsetSet(is,intrev32ifbe(is->length),value);
这是插入的部分，我开始一直没看明白，要插入新元素并且要保存排序，难度不应该在上面一个个挪元素的时候比一下大小来确定差那儿么？
为啥小于0就插入在头部大于等于0就插入尾部，万一有数比它更小或者更大呢?这里后来仔细琢磨就明白了
1. 只有在encoding变大时才执行这个函数
2. encoding变大也就是16到32/64, 或者32到64, 也就意味着插入的这个数比当前的所有的都大(如果是正数)，或者都小(负数).
3. 所有简单判断下，小于0就是最小的插在头部，大于0就是最大的，插在尾部

### Reference:
1. [github: redis intset](https://github.com/antirez/redis/blob/unstable/src/intset.c)
