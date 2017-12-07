+++
title = "bloom filter"
date = "2017-09-17"
categories = ["Linux"]
tags = ["bloomfilter", "hash"]
Authors = "Xudong"
draft = true
+++

### bloomfilter介绍
有很多类似的使用场景，比如chrome里面判断恶意站点，简单的方法，用一个类似集合的方法把所有的恶意网址存起来，然后
每来一个url比对一下，看在不在里面，这样做没有什么问题，只不过随着恶意站点数量的增加，这个集合的占用空间越来越大。

### bloomfilter的处理方式
**需要注意bloomfilter有一定的false posivite**, 也就是当它判断元素在filter里的时候，有一定概率该过滤器里实际上没有
该元素。

通过需要存储的站点数量n和期望的一个错误率p，计算出需要的bit位数量m和hash函数的个数k。首先初始化一个m bit的全0数组，
然后对于每个需要添加进去的元素，我们计算出k个独立的hash值, 然后映射到0到m(通常直接做模运算，hash % m)，然后将对应
的bit设置1.

在查询的时候，通常的计算元素的k个hash值，映射到0-m, 然后去过滤器那里查对应槽位的值，如果k个槽位里任何一个为0，那么
这个元素肯定不在过滤器里；但是如果k个槽位全部是1，并不能保证这个元素一定在过滤器里。假设一个元素`hello world`, 映射
到10, 20, 40这三个槽位，假设三个槽位都是1，但是很可能10，20为1是因为加入了`aaaa`, 40为1可能是因为之前加入了`bbb`造成
的。

所以bloomfilter判断的不存在是100%确定的，判断存在是有一定的误判概率的，我们可以通过配置k和m来试这个误判率控制在一个
比较小的值。具体的计算过程和背后的原因可以看wiki的详细推导。

1. 通过hash算法获取2个初始值，通过`k_i = h1 + h2 * i`来获取k个hash值，节省hash时间，而且均等性可靠。
2. 对于python，通常将bit array映射到byte array(8bit=1byte)

### 数据库
1. [wiki bloomfilter](https://en.wikipedia.org/wiki/Bloom_filter)
2. [cassandra bloomfilter implementation](https://github.com/apache/cassandra/blob/trunk/src/java/org/apache/cassandra/utils/BloomFilter.java)
