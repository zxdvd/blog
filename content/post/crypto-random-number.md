+++
title = "随机数"
date = "2017-09-10"
categories = ["algorithm"]
tags = ["crypto", "random"]
Authors = "Xudong"
draft = true
+++

1. 要注意rand % N 的结果分布是不均匀的
比如rand结果是0到255，模100，其中0到55出现的概率比56到99高，以55为例，55,155,255模100都是55，但是对于56，只有56，156模100这两种
情况模100结果是56.

2. 关于随机数要注意数理上的随机和加密用途的随机上的差异，前者更强调概率均等，后者通常更强调不可预测性. 对于sessionId，token之类的
我们应该用crypto模块提供的rand功能。

3. 关于nanoid这个库
随机生成长为N的字符串，字符集是`a-zA-Z0-9~_`共64个，注意这里特意64个是有原因的。
生成的方法是:
*. 由库(crypto.randomBytes)或者浏览器API(crypto.getRandomValues)来生成N字节的字符
*. 对于生成的N字节，对每个字节取低6位(`byte & 63`), 低六位的值范围正好是0-63共64个，映射到之前的64个字符上

这个库还支持自定义字符集，比如我想用`a-z0-9`共36个字符，那怎么处理呢？
*. 找到最近的2^n-1的数mask，要比字符集长度大的，比如对于36来说，最近的是63.
*. 同样的由库生成随机字节，对每个字节与mask做与(`&`)运算，这个运算结果肯定在`0-mask`之间，这个数如果小于36，那么我们映射到之前的36个字符集上去，
相当于生成了一个，这个数如果大于36，那么忽略掉，继续从系统生成的随机字节里取一个，继续同样的步骤

所以对于这样的任意的字符集，如果mask不能正好是2^n-1，那么有一定概率生成的随机字节是不可用的，所以注意到nanoid库里对最开始生成随机字节
的时候乘以了一个系数(1.6)应该是考虑到这样的情况的

如果参照附录1文章里的思路，nanoid这个库是有不少的优化空间的，以默认的64个字符集为例，系统生成的每一个字节有8位，而我们只用了低六位，另外
2位是浪费了，其实是可以充分利用的。那么其实就是系统生成一个字节流，我们按照64进制读取出来然后做字符集映射就可以了。


### References
1. [stackoverflow generate random string of fixed length](https://stackoverflow.com/questions/22892120/how-to-generate-a-random-string-of-a-fixed-length-in-golang)
2. [node crypto](https://gist.github.com/joepie91/7105003c3b26e65efcea63f3db82dfbac)
3. [js nanoid](https://github.com/ai/nanoid)
