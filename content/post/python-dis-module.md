+++
title = "python dis module"
date = "2017-09-26"
categories = ["Linux"]
tags = ["python"]
Authors = "Xudong"
draft = true
+++


在wtfpython里发现了一个奇怪的问题，考虑一段简单的代码，如下:
``` python
a = [1, 2, 3]
b = [10, 20, 30]
g = (i + j for i in a for j in b if 4 in a and 100 in b)
a = [4, 5, 6]
b = [100, 200, 300]
print(list(g))

>>>[101, 201, 301, 102, 202, 302, 103, 203, 303]
```
这个结果非常奇怪
1. 输出了9个值，那么看起来if里面的`4 in a`和`100 in b`都是True，那么似乎`list(g)`执行的时候的a和b是后面
再次赋值的`[4, 5, 6]`和`[100, 200, 300]`, 但是我们仔细看看输出的结果，从值来看似乎`a=[1, 2, 3]; b=[100, 200, 300]`，
这里面就很矛盾了, 看起来两个b都是后来第二次重新赋值时候的b，但是两个a一个是第一次赋值的一个是第二次赋值的。

这个时候我们可以用dis模块看看他们生成的字节码，或者能有些线索。
``` python
a = [1, 2, 3]
b = [10, 20, 30]
g = (i + j for i in a for j in b if 4 in a and 100 in b)
a = [4, 5, 6]
b = [100, 200, 300]
import dis
dis.dis(g)

>>>
  1           0 LOAD_FAST                0 (.0)
        >>    3 FOR_ITER                52 (to 58)
              6 STORE_FAST               1 (i)
              9 LOAD_GLOBAL              0 (b)
             12 GET_ITER
        >>   13 FOR_ITER                39 (to 55)
             16 STORE_FAST               2 (j)
             19 LOAD_CONST               0 (4)
             22 LOAD_GLOBAL              1 (a)
             25 COMPARE_OP               6 (in)
             28 POP_JUMP_IF_FALSE       13
             31 LOAD_CONST               1 (100)
             34 LOAD_GLOBAL              0 (b)
             37 COMPARE_OP               6 (in)
             40 POP_JUMP_IF_FALSE       13
             43 LOAD_FAST                1 (i)
             46 LOAD_FAST                2 (j)
             49 BINARY_ADD
             50 YIELD_VALUE
             51 POP_TOP
             52 JUMP_ABSOLUTE           13
        >>   55 JUMP_ABSOLUTE            3
        >>   58 LOAD_CONST               2 (None)
             61 RETURN_VALUE
```

这是g这个generator对应的字节码，dis.dis可以应用于object，class，function，module等。

第一列的1是行号，如果我们`dis.dis(xx_function)`之类的多行的一个东西，这一列会有对应的行号。第二列是字节偏移，
这里实际是是把字节码转义后给我们的一个非常友好的输出，实际上的字节码就是一段二进制字节流，每个指令的字节长度
不一样的，有的有参数有的没有，所有第二列记的是偏移，比如第0字节是LOAD_FAST, 第3字节是FOR_ITER。第三列是对应的
指令名称。第四列是指令参数。

我们先看字节偏移为9，22，34的LOAD_GLOBAL，从字面意思也知道，这个是从全局变量里取值，这就解释了为啥第二个a以及
两个b用的是第二次赋值的值了，因为他们是运行时获取的，而不是generator定义时绑定的。

我们再看第一个指令`0 LOAD_FAST 0(.0)`, 这里没有`LOAD_GLOBAL 0 (a)`, 这是因为在定义的时候a的引用已经传给generator
了，变成一个local的变量了，所以用的是`LOAD_FAST`, 我没有找到具体的规定或者解释，但是我的理解就是generator里的第一层
迭代(这里的`for i in a`)里的a是在定义的时候传引用给generator的，其它的变量都是运行时获取的。

这样也就解释了下面的这个问题
``` python
a = 10
b = None
g = (i for i in a for i in b)
```
这个代码第三行就报错了，也就是generator定义的时候是会进行检查的，我觉得它检查的目的就是因为它在定义的时候需要绑定
第一次迭代，不然就没有必要在定义的时候去检查了。

这个问题，最终弄清楚了为什么输出那样的结果，也知道了generator在定义的时候只有第一层的`for i in xx`是定义时传应用的，
但是没找到关于这部分的一个官网的定义和解释，后面有时间会继续研究下。


### References
1. [python doc: dis module](https://docs.python.org/3/library/dis.html)
2. [a blog about python byte code](http://akaptur.com/blog/2013/11/17/introduction-to-the-python-interpreter-3/)

