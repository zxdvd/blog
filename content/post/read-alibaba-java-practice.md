+++
title = "读书笔记: 阿里巴巴java开发手册"
date = "2017-10-16"
categories = ["Linux"]
tags = ["database"]
Authors = "Xudong"
draft = true
+++

#### 数据库设计
我对java没什么了解，不过数据库和工程部分可以参考下。
1. bool类型用is_xxx命名。
2. 表名字段名不要包含大写字母，一是一些文件系统大小写不敏感，二有些数据库本身大小写不敏感，大写的字段必须用引号
，如"firstName".
3. 字段名注意跟数据库的关键词保留字区分，如`default`, `type`.
4. 小数用decimal模块，不要用float或者double.( float或者double有精度损失)，代码里有对decimal进行技术也需要用decimal
的模块，不要会有精度损失，最好不要直接等于或不等于，可以类似于这样`if abs(v1 - v2) <= 1e6`
5. 如果存的字符串长度几乎相等，用char定长类型(更容易定位查找，但是注意会有padding)
6. varchar长度不要超过5000，太长了使用text类型，独立一个表，避免影响其它字段的索引效率。
7. 字段允许适当冗余以提高查询性能, 但是该字段不应是频繁变动的字段，不应是特别长的字符串或者text
8. 单表超过500万行或者容量超过2GB才考虑分库分表的事。

#### 数据库索引
1. 唯一索引好过应用层查重再insert。
2. 禁止超过3个表的join，我觉得这个看业务吧，被关联的字段需要有索引。
3. varchar上的索引，必须指定前缀长度，没必要对全字段建立索引，根据文本实际区分度建立索引。
`select count(distinct left(xxx_field, length)) / count(*) from xxx_table`, 可以调整length大小来找到合适的值。
4. 左模糊或者前模糊没发利用索引。
5. 创建索引要避免以下误解:
        1. 宁滥勿缺，认为一个查询就需要建一个对应的索引；
        2. 宁缺勿滥，认为索引消耗空间，影响新增，更新和删除
        3. 抵制唯一索引，认为需要在应用层通过先查后插方式解决
6. null与任何值比较结果都是null, null与null的等于或不等于判断结果还是null
7. 不得使用外键与级联，外键和级联更新适用于单机低并发，不适合分布式，高并发集群; 外键影响数据库插入速度
8. 禁止使用存储过程，存储过程难以调试和扩展，没有移植性(对于小公司，换了个人难以维护，懂这个的程序相对少一些)
9. 字符串都用utf8， 表情可以用utfmb4, 注意length, character_length差别
10. truncate table比delete快，但是无事务且不触发trigger

### 数据库
1. [github: 阿里巴巴java开发手册pdf](https://github.com/alibaba/p3c/blob/master/%E9%98%BF%E9%87%8C%E5%B7%B4%E5%B7%B4Java%E5%BC%80%E5%8F%91%E6%89%8B%E5%86%8C%EF%BC%88%E7%BB%88%E6%9E%81%E7%89%88%EF%BC%89.pdf)
