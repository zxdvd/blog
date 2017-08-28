+++
title = "配置travis自动更新github page"
date = "2017-08-05"
tags = ["algorithm", "huffman code"]
slug = "huffman-code"
Authors = "Xudong"
+++

我主要参考附录里的两篇文章来学习huffman code的，特别是第一篇，介绍的非常详细。

这个算法可以用来按照词频对文本进行压缩，比如默认的ascii编码的文本，每个字符都占一个字节，但是各个
字符出现的频率大不一样，可以设计一种映射，把出现的比较高的字符用更少的bit来表达，比如'e'用001, 'a'用010,
' '用101，不常用的'z'可能就是`100001`等等，通过这样的方式实现压缩。

这里的关键是怎么构建这样的一个映射关系呢？不能瞎编，特别是必能有歧义，必须要唯一，比如"01, 00, 010"这样的
就不是一个可用的映射了，因为如果bit流出现了01，它不知道这个是代表`01`本身还是作为`010`的前缀。

所以这里的关键是要形成一个前缀树，而且要确保高词频的字符的高度低，这样才能实现最大的压缩率。

<!--more-->

总的步骤

1. 获取词频
2. 跟进词频得到一个前缀树
3. 根据这个前缀树得到字符的映射表
4. 根据映射表将字符流转换成新的变长bit流
5. 还需要把这个映射表想办法放在文件前面，没有映射表就没发解码了
6. 有的文件系统的文件大小是有规定的，比如最小单位是字节之类的，这样生成的bit流可能不是字节整数倍，这时候需要padding，
但是padding了之后会导致解码的时候不知道那里是正常的结束那里是padding，所以需要一个特别的标示来标示结尾，这个标示应该
也在前缀树里有表达，很显然它的高度应该是最大的(因为出现频率最小)
7. 在解码的时候先根据文件前面的特别信息获取映射表生成前缀树，然后读取字节流从前缀码中查找出对应的字符，找到碰到那个特别的标示结尾的编码为止。


他的工作流程大致如下:

```
push OR pull request to a repo ->
    github notify this event to travis ->
        travis clone your code and build according the configuration.
```

##＃ Reference:

1. [very detailed introduction about huffman code](https://www.cs.duke.edu/csed/poop/huff/info/)
2. [nice one about huffman code](http://www.geeksforgeeks.org/greedy-algorithms-set-3-huffman-coding/)