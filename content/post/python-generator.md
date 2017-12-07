+++
title = "python generator"
date = "2017-09-20"
categories = ["Linux"]
tags = ["python"]
Authors = "Xudong"
draft = true
+++


关于generator以及相关东西的一些笔记，都是在3.5以上版本测试的。

```
def gen(n):
        for i in range(n):
                yield i

a = gen(10)
print(next(a))
print(next(a))
print([i for i in a])
```

如上是一个很简单的generator，使用next调用或者for迭代获取yield返回的值.


1. yield and return in same function
如果函数里既有yield又有return，是什么效果？
``` python
def gen(n):
        for i in range(n):
                if i > 4:
                        return 10
                yield i

print([i for i in gen(6)])
```
可以看到输出结果是`[0, 1, 2, 3, 4]`而不是`[0, 1, 2, 3, 4, 10]`, return的作用跟`StopIteration`
差不多，让generator终止了。

``` python
def gen1():
        return 10
        yield 20

print(gen1())
print([i for i in gen1()])
```
猜猜上面这个结果如何？
看起来，yield 20根本没有机会执行，感觉跟`def gen1(): return 10`一样啊。实际上不一样，python运行
的时候检测到函数里有yield，它就认为这个函数是generator了，那么第一个结果显然不会返回10了，而是
一个generator对象，第二个是一个空的list。如果我们直接`next(gen1())`, 会发现它直接抛`StopIteration`
异常了。

``` python
def gen2():
        a = yield from range(3)
```
类似于上面的函数不会得到想要的`[0, 1, 2]`, 同上面，只要函数里有`yield`这就是个generator函数，比如
``` python
def gen2():
        a = yield from range(3)
        print('i am here', a)
        yield from range(3, 0, -1)

print([i for i in gen2()])

>>>
i am here None
[0, 1, 2, 3, 2, 1]
```

`yield from`相当于`for i in generator: yield i`. 如果我们想在常规函数里里使用可以定义个helper函数
``` python
def yield_from(g):
        for sub_g in g:
                yield sub_g

list(yield_from(range(4)))
```

2. generator send
``` python
def gen3(n):
        while n < 10:
                try:
                        n += yield n
                except Exception as e:
                        print(e)
                        n = 1

a = gen3(3)
print(next(a))
print(a.send(2))
print(a.send('hello world'))
```
注意一开始得用`next(a)`或者`a.send(None)`让generator开始执行.







Non-cryptographic hash is very fast while cryptographic hash is slow.

non-cryptographic hash
fnv (very easy)
murmur (used in redis and cassandra)
hashxx
very fast so easy to brute force

cryptographic hash
sha md5


### References
1. [very very nice slides about generator and coroutine by Dave Beazley](http://www.dabeaz.com/coroutines/Coroutines.pdf)

