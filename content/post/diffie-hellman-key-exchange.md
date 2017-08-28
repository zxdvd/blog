+++
title = "crypto: Diffie-Hellman Key Exchange"
date = "2017-07-21"
categories = ["Linux"]
tags = ["crypto"]
slug = "diffie-hellman-key-exchange"
Authors = "Xudong"
draft = true
+++

对于一些函数y=f(x)，已知f和x求y是很快的，但是已知y和f求x就非常困难，或者耗时很长(比如最强大的计算机也需要几百年几千年)，这样的一些f可以用来进行加密。

附录1的视频里又一个简单的demo，假设A和B要通信，C可以窃听到他们交换的全部明文信息，怎么才能安全的传递一个信息是A，B都知道但是C猜不出来呢？

视频中以颜色为例，两种颜色混合可以得到第三张颜色，但是只告诉你混合后的颜色，你很难知道它是那几种颜色混合得到的，这就是之前说到的那个f函数了，你穷举可能需要很久。

<!--more-->

首先A，B随便选一个公共的颜色，比如color1，然后分别选一个其他颜色，只有自己知道就行了，假设A的是color2，B的是color3. A，B分开将公共的颜色和自己私有的颜色混合，A得到color1color2，B得到color1color3，A，B分开将混合后的颜色传递给对方，然后A，B分别将收到的这个颜色跟自己私有的混合得到新的颜色，这时候每个人知道的颜色如下

|A------------------|C-----------------|B---------------------|
|-------------------|------------------|----------------------|
|color1             | color1           |  color1              |
|color2             |                  |  color3              |
|color1color2       | color1color2     |  color1color3        |
|color1color3       | color1color3     |  color1color2        |
|color1color3color2 |                  |  color1color2color3  |

我们可以看到A和B最后得到的color1color3color2和color1color2color3实际上是一样的，并且这个颜色是中间的C所不能得到的，C手中的三种颜色无论怎么混合都是得不到color1color2color3的(C如果把color1color3和color1color2混合得到的是color1color1color2color3，是不一样的颜色)。这样就实现了A和B交换信息得到共有的一个密文，并且C无法破解。以为他们可以以这个密文为基础进行加密通信。

上面这个只是一个通俗的例子，视频后来也说了实际上额使用，如下

|A------------------------------|C-------------------|B---------------------|
|-------------------------------|--------------------|----------------------|
|public number: 3, 17           |3, 17               |3, 17|
|private number 13              |                    |private number 15|
|12 = 3^13 mod 17               |                    |6 = 3^15 mod 17|
|get 6 from B                   |6, 12               |get 12 from A|
|secret = 6 ^ 13 mod 17         | ???                |secret = 12 ^ 15 mod 17|

6 ^ 13 mod 17 = (3^15 mod 17) ^ 13 mod 17 = (3^15^13) mod 17
(3^13 mod 17) ^15 mod 17 = 12 ^ 15 mod 17 = (3^13^15) mod 17

其实主要是依赖下面的数学定理

>>>
((A ^ x1) mod P) ^ x2 mod P = (A ^ x1 ^ x2) mod 17 = ((A ^ x2) mod P) ^ x1 mod P
>>>

C手上只有(A ^ x1) mod P和(A ^ x2) mod P是没发算出(A ^ x1 ^ x2) mod P的。

### References
1. [Public key cryptography - Diffie-Hellman Key Exchange](https://www.youtube.com/watch?v=YEBfamv-_do&feature=youtu.be)

