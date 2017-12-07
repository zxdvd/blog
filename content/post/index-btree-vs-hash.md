+++
title = "index: btree vs hash"
date = "2017-10-08"
categories = ["Linux"]
tags = ["database", "btree", "hash"]
Authors = "Xudong"
draft = true
+++


1. 现在的btree索引实现一般都是叶子节点存数据，非叶子节点只是separator，通常把非叶子节点全部缓存在内存里，所以对于
查询来说磁盘IO跟hash一样的，就一次。

2. hash索引的扩容是一个很麻烦的事情，很多key需要重算和重写到新的位置。

3. 对于联合索引，比如 (first_name, last_name), btree支持前缀搜索，即可以支持仅有first_name，也可以(first_name, last_name), 但是对于hash索引，只能支持后一种。

4. 在创建索引的时候，如果先排序再创建，btree会很快(顺序IO); 而hash就算数据排序了，插入也是乱序的，IO性能会很差。

5. hash无法支持排序和范围查找，btree就没有问题。


1. 对于事务，rollback后能保证数据不变，但是底层的数据结构很可能发生改变了，比如节点合并或者分裂或者重新分布了，
这是正常的.

### 数据库
1. [wiki bloomfilter](https://en.wikipedia.org/wiki/Bloom_filter)
2. [cassandra bloomfilter implementation](https://github.com/apache/cassandra/blob/trunk/src/java/org/apache/cassandra/utils/BloomFilter.java)
