+++
title = "github: wtfpython"
date = "2017-09-23"
categories = ["Linux"]
tags = ["python"]
Authors = "Xudong"
draft = true
+++


这是一个不错的介绍python里的一个hack/tricky的地方的项目，里面有不少值得注意的地方，我根据我自己的理解
介绍下。

项目地址 [wtfpython](https://github.com/satwikkansal/wtfpython)

本文对于的commitid是`6a1bbc9`


1. int float hash
``` python
s = {}
s[5] = 'js'
s[5.0] = 'python'
print(s)
```

可以发现5.0这个key覆盖了5，python里字典的key是必须可hash的，字典判断两个元素是否相同是判断值以及hash值是否相同的。
``` python
i = 0
class A(int):
    def __hash__(self):
        global i
        i += 1
        return i

b = {}
a1 = A(10)
b[a1] = 30
b[a1] = 40
print(b)
```
上面这个例子，我们从int类型继承一个class出来，简单改变下它的hash函数，确保对于相同的实例，他们的hash值也不一样，
通过结果我们发现，果然同一个实例在字典里是两个key了，因为他们的hash不一样。

``` python
class B(int):
    def __hash__(self):
        return 1

    def __eq__(self, other):
        return True

b[B(5)] = 5
b[B(6)] = 6
print(b)
print('equal: ', B(5) == B(6))
```
这个例子，我们重载了hash和eq函数，确保对于任何int值，他们的eq和hash判断都是相同的，果然在字典里只有一个key，后一个
会覆盖前一个。

所以字典和set的唯一判断是依据hash和eq的判断来的，跟是同一个实例或者不是同一个实例都没有关系的.


2. True, False, 1, 0
``` python
print(True == 1, False == 0)
print(isinstance(True, int))
print(isinstance(1, bool))
print(issubclass(bool, int))

a = {}
a[True] = 1
a[1] = 2
a[1.0] = 3
```
True和False是由int派生出来的, 根据第一条，hash和eq比较一样的话，对于dict来说是同一个key了，所以这个问题就很好解释了。

2. 定义时计算以及运行时计算
这个我也不知道怎么翻译，这个我之前也没碰到过，不过看了解释就明白了。
``` python
a = [1, 2, 3]
g = (i for i in a)
a.append(10)
a = [4, 5, 6]
print(list(g))
```
通过这个例子，我们发现generator里的`for i in xx`在定义的时候应该是传引用的，后面对a重新赋值对它没有影响了, 但是
append起作用了。

``` python
a = [1, 2, 3]
b = [10, 20, 30]
g = (i + j for i in a for j in b)
a = [4, 5, 6]
b = [100, 200, 300]
print(list(g))
```
另一个例子，我自己发现的，暂时还没有解释，`for i in a for j in b`, 这里似乎a是定义的时候绑定的，但是几乎同样的b
又似乎是运行时绑定的。

``` python
array = [1, 8, 15]
g = (x for x in array if array.count(x) > 0)
array = [2, 8, 22]
print(list(g))
```
这个是repo里面的那个例子，综合看起来，感觉就是只有第一个`for i in xx`是传引用，后面的for或者if之类的都是值传递。
关于这个我有个分析，会在另一篇文章里写下。

3. raw string末尾的反斜杠
``` python
print("\\ some string \\")
print(r"\ some string")
print(r"\ some string \")
```
这里第三个会报`SyntaxError`，我感觉这更像是python的bug，解释说这里python在解析的时候开始是没有什么特别处理的，对于
每个反斜杠还是会去找后面的一个字符，没找到就报语法错误了。
[python官方faq解释](https://docs.python.org/3/faq/design.html#why-can-t-raw-strings-r-strings-end-with-a-backslash)

4. 字符串拼接性能
直接给文章的结论吧，`''.join()`是性能最好的，然后是`+=`性能高于`s1 + s2 + s3`这样的。

5. is和==
``` python
a = 256
b = 256
a is b
a = 257
b = 257
a is b
```
这个例子没啥，多数人应该都知道了，就是常用小数缓存池，而且这个大小是可以调整的，这个很多其它语言也有的。

``` python
>>> a, b = 257, 257
>>> a is b
True
id(a) == id(b)
>>> id(a) == id(b)
True
```

这里就有些奇怪了，继续看下面的我自己扩展的一些例子
``` python
>>> a, b = [1], [1]
>>> a is b
False
>>> a, b = (1), (1)
>>> a is b
True
```
按照repo里的说法，同一行定义的相同的值会指向同一个对象，但是似乎仅限于不可变对象如int, string, tuple, 对于list就不是这样的。
注意这里是在交互式shell里的行为，在python文件里又是另外一种情况了，不同行的同样的不可变对象也是指向同一个对象的。


``` python
>>> dis.dis('a, b = 400, 400')
  1           0 LOAD_CONST               2 ((400, 400))
              3 UNPACK_SEQUENCE          2
              6 STORE_NAME               0 (a)
              9 STORE_NAME               1 (b)
             12 LOAD_CONST               1 (None)
             15 RETURN_VALUE
```


``` python
>>> dis.dis('a, b = [1], [1]')
  1           0 LOAD_CONST               0 (1)
              3 BUILD_LIST               1
              6 LOAD_CONST               0 (1)
              9 BUILD_LIST               1
             12 ROT_TWO
             13 STORE_NAME               0 (a)
             16 STORE_NAME               1 (b)
             19 LOAD_CONST               1 (None)
             22 RETURN_VALUE
```

6. 闭包的变量绑定
``` python
funcs = []
for x in range(7):
        funcs.append(lambda: x)

[i() for i in funcs]
>>> [6, 6, 6, 6, 6, 6, 6]
```
这问题js里面也存在，闭包的时候绑定的是变量的作用域和名字，在使用的时候根据作用域和名字找到x的时候x的值就是6.
解决的办法很多，如在定义的时候把x放入另一个作用域
``` python
for x in range(7):
        def f(i):
                return lambda: i
        funcs.append(f(x))
```
[stackoverflow上的问题](https://stackoverflow.com/questions/233673/how-do-lexical-closures-work)
[stackoverflow上的另一个问题](https://stackoverflow.com/questions/2295290/what-do-lambda-function-closures-capture)


7. 乘号与浅拷贝
``` python
a = [ [] ]
b = a * 3
a[0].append(1)
print(b)
```
这里实际上`*`是浅拷贝，说到浅拷贝那么这里就没什么可说的了，很正常的行为。js里也有类似的问题，比如`Array(4).fill({})`
实际上是个浅拷贝。

这里`*`是调用了list的`__mul__`方法，具体代码可以看[这里](https://github.com/python/cpython/blob/master/Objects/listobject.c#L513)
代码对list长度为1特别处理了下，大于1的核心代码如下，`*p = items[j];`这里可以看出是浅拷贝.
``` C
for (i = 0; i < n; i++) {
    for (j = 0; j < Py_SIZE(a); j++) {
        *p = items[j];
        Py_INCREF(*p);
        p++;
    }
}
```

8. 函数的default值
这里在另一篇文章里单独说过，一切皆对象，函数的默认值也是函数这个对象的一部分，注意看`f.__defaults__`就明白了。

9. add和iadd
``` python
a = [1, 2, 3, 4]
b = a
a += [5, 6, 7, 8]
print(a, b)
a = a + [9, 10]
print(a, b)
```
看完结果就明白了，`+`和`+=`实际上是有区别的，它们分别调用的是`list.__add__`和`list.__iadd__`, 一个是生成一个新的list，
一个是在原list上extend。看上面那个`list.object.c`里面的源代码大致可以看到，`iadd`也就是`list_inplace__concat`里面调用了
基本就是调用的`list_extend`, 然后`list_extend`里面用了`list_resize(self, m + n)`开扩容的；而`add`也即是`list_concat`里面
就是`size = Py_SIZE(a) + Py_SIZE(b); np = (PyListObject *) PyList_New(size);`生成了一个新的对象。


10. 关于元组(tuple)
``` python
t = ('one', 'two')
for i in t:
    print(i)

t = ('one')
for i in t:
    print(i)

t = ()
print(t)
```
再补充一个例子
``` python
a = (1)
b = 1
print(a == b)
print(type(a))
```

实际上`(1)`跟`1`是一样的, `('one')`跟`'one'`也是，它并不是**包含一个元素的元组**的意思，如果要表示包含一个元素的元祖应该这样
`(1, )`或者`1,`，如果想表达空元组用`()`.


一些其它的我觉得属于错误使用的情况，我觉得应该在代码中注意避免，比如
1. 空格和tab混用导致的奇葩行为
2. 在iterate dict或者list的时候，对原dict，list有修改，这些应该避免的
3. iterate的时候包含赋值操作，如`for i, some_dict[i] in enumerate('wtf')`
4. datetime.time() bool判断为False这个bug在python 3.5中修复了


### References
1. [github repo wtfpython](https://github.com/satwikkansal/wtfpython)

