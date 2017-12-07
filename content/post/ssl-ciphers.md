+++
title = "openssl cipher suites"
date = "2017-09-05"
categories = ["Linux"]
tags = ["nginx", "ssl", "https"]
slug = "openssl-cipher-suites"
Authors = "Xudong"
draft = true
+++


今天别的项目的同事说他们的首页在win7下360无法方法，页面被360给定向到自己的页面去了，帮着查了下原因，我自己用curl和openssl
简单试了下是没有问题的，感觉应该是跟系统有关系的，那台windows访问我们其他的网站的https页面是正常的，我没有windows测试，
不过根据这些信息我感觉跟我们nginx的配置有关系。

开始怀疑是NPN和ALPN支持的问题，后来排除了，我对比正常网站和这个不正常的网站发现除了ssl的key不一样，另外`ssl_ciphers`设置差别
很大，这个有问题的网站ciphers支持太少了(`ssl_ciphers AES256+EECDH:AES256+EDH:!aNULL;`)，把正常网站的那一串ciphers抄过来就好了。

问题结束了，顺便多了解下关于openssl ciphers以及应用程序配置的问题。

<!--more-->

在ssl握手阶段, client要发送一些基础信息给server，其中包含自己支持的cipher suites列表，server会选一个自己支持的发送给client，这样
双方就确定了这个cipher了。那么cipher是干嘛的？ 一个cipher suite包含一个密钥交换算法，一个加密算法，一个摘要算法,
如`ECDHE_RSA_WITH_AES_128_GCM_SHA256`里, `ECDHE_RDA`是密钥交换算法，`AES_128_GCM`是对称加密算法, `SHA256`是摘要算法。tls1.3里这些
定义有所改变.

查看系统openssl支持的ciphers
``` shell
$openssl  ciphers -v
ECDHE-RSA-AES256-GCM-SHA384 TLSv1.2 Kx=ECDH     Au=RSA  Enc=AESGCM(256) Mac=AEAD
ECDHE-ECDSA-AES256-GCM-SHA384 TLSv1.2 Kx=ECDH     Au=ECDSA Enc=AESGCM(256) Mac=AEAD
ECDHE-RSA-AES256-SHA384 TLSv1.2 Kx=ECDH     Au=RSA  Enc=AES(256)  Mac=SHA384
ECDHE-ECDSA-AES256-SHA384 TLSv1.2 Kx=ECDH     Au=ECDSA Enc=AES(256)  Mac=SHA384
ECDHE-RSA-AES256-SHA    SSLv3 Kx=ECDH     Au=RSA  Enc=AES(256)  Mac=SHA1
ECDHE-ECDSA-AES256-SHA  SSLv3 Kx=ECDH     Au=ECDSA Enc=AES(256)  Mac=SHA1
SRP-DSS-AES-256-CBC-SHA SSLv3 Kx=SRP      Au=DSS  Enc=AES(256)  Mac=SHA1
SRP-RSA-AES-256-CBC-SHA SSLv3 Kx=SRP      Au=RSA  Enc=AES(256)  Mac=SHA1
SRP-AES-256-CBC-SHA     SSLv3 Kx=SRP      Au=SRP  Enc=AES(256)  Mac=SHA1
DH-DSS-AES256-GCM-SHA384 TLSv1.2 Kx=DH/DSS   Au=DH   Enc=AESGCM(256) Mac=AEAD
DHE-DSS-AES256-GCM-SHA384 TLSv1.2 Kx=DH       Au=DSS  Enc=AESGCM(256) Mac=AEAD
DH-RSA-AES256-GCM-SHA384 TLSv1.2 Kx=DH/RSA   Au=DH   Enc=AESGCM(256) Mac=AEAD
DHE-RSA-AES256-GCM-SHA384 TLSv1.2 Kx=DH       Au=RSA  Enc=AESGCM(256) Mac=AEAD
DHE-RSA-AES256-SHA256   TLSv1.2 Kx=DH       Au=RSA  Enc=AES(256)  ｀3Mac=SHA256
DHE-DSS-AES256-SHA256   TLSv1.2 Kx=DH       Au=DSS  Enc=AES(256)  Mac=SHA256
DH-RSA-AES256-SHA256    TLSv1.2 Kx=DH/RSA   Au=DH   Enc=AES(256)  Mac=SHA256
DH-DSS-AES256-SHA256    TLSv1.2 Kx=DH/DSS   Au=DH   Enc=AES(256)  Mac=SHA256
DHE-RSA-AES256-SHA      SSLv3 Kx=DH       Au=RSA  Enc=AES(256)  Mac=SHA1
...
```

还有一点容易忽视的是，certificate里也包含了public key的算法，这个我们在制作证书的时候有指定的，包含算法类别和长度。所以，在ssl握手阶段
cipher由客户端列表，服务端列表以及证书公钥算法类型来确定一个cipher suite，如果确定不了，比如client和server的ciphers没有交集，或者证书是
ECDSA的但是client列表没有包含ECDSA的cipher，这样握手就失败了，最开始同事那边就是这个问题导致的。

为什么需要这么多ciphers呢？我觉得主要是满足不同的需求以及要支持各种新的老的不同平台的不同应用造成的。
1. 需求方面，有的人对加密以及安全性要求高，可以选择高强度的cipher，相应的cpu压力会大一些；有的不需要那么强，也希望cpu负荷小，有的希望
适中一些。
2. 软件开发过程中，一些旧的算法被发现有漏洞或者强度不够，但是老旧系统比如xp，比如android 4.0之类的还在使用，如果需要支持这些系统，server的
ciphers需要包含对它们的支持。


server上是可以有多种不同类型的证书的，我们可以根据需要给client端不同的证书，nginx从1.11开始支持同一个站点多个不同类型证书的支持
([设置方法](https://scotthelme.co.uk/hybrid-rsa-and-ecdsa-certificates-with-nginx/))
我们可以这样测试一个网站支不支持某个cipher
``` shell
$ openssl s_client -cipher 'ECDH+AESGCM'  -connect www.baidu.com:443  -servername baidu.com | openssl x509 -noout -text
```
也可以更具体的`-cipher 'ECDHE-RSA-AES128-GCM-SHA256'`

### 前向保密(Perfect Forward Secrecy)
在前面的介绍中，我们知道握手之后的数据都是用对称加密传输的，对称加密的密钥是client短用server的public key加密传送给server，server的私钥可以
解密它，这意外着，如果你监听了整个会话并且通过某种方式获得了密钥，你就可以解密整个通信内容了。

那么如果有个人/公司/政府一只监听ssl通信并保存，然后通过其他渠道获取了私钥，这些历史通信就很危险了，要知道获取私钥的难度比破解对称加密应该是
容易得到(各种系统漏洞，管理漏洞)，针对这样的情况支持PFS的cipher就出现了。

目前支持PFS的都是基于DH(Diffie-Hellman)的cipher，这种cipher对每个session都会生成一个临时的私钥，在session结束后就销毁了，那么就没有上面的问题了。
当然任何事情都是有代价的，DHE和ECDHE与RSA结合会增加不少CPU负荷，如果与ECDSA结合增加的没有那么明显，但是目前很多老平台老client不支持ECDSA.

需要注意的是对于TLS1.2，如果使用了session ticket，那么对称加密密钥会加密存在client段，一旦服务器的加密密钥丢失，通过解密session ticket可以获取加密密钥进而就可以
解密以这个key加密的流量了，也就是**session ticket会破坏PFS**.


在ciphers的设置上，如果client和server都是可控的，我们可以根据自己的实际情况设置一个性能和安全都不错的cipher，但是如果是web用的，比如nginx，
haproxy之类的，我们就需要通过调整优先级来适应各式各样的client了。


### 名词解释
- [AES和AES(GCM)](https://crypto.stackexchange.com/questions/2310/what-is-the-difference-between-cbc-and-gcm-mode) 前者通常就是AES CBC, 后面一个是比较新的，带有MAC功能的AES，抗攻击能力很好一些，128bit的AES-GCM安全性跟256bitAES-CBC相当.
- [ECDSA](https://en.wikipedia.org/wiki/Elliptic_Curve_Digital_Signature_Algorithm) 一种基于椭圆曲线密码学的签名算法，256bit的安全性与3248bit RSA相当，性能更好(同等安全性)，还可以减小证书大小

### References
1. [ssl handshake](https://www.ssl.com/article/ssl-tls-handshake-overview/)
2. [ssl ciphers](https://thycotic.com/company/blog/2014/05/16/ssl-beyond-the-basics-part-2-ciphers/)
3. [stackexchange certificate type](https://security.stackexchange.com/questions/7440/what-ciphers-should-i-use-in-my-web-server-after-i-configure-my-ssl-certificate)
4. [harden web server 2017](https://hynek.me/articles/hardening-your-web-servers-ssl-ciphers/)
5. [PFS introduction](https://www.namecheap.com/support/knowledgebase/article.aspx/9652/38/perfect-forward-secrecy-what-it-is)
6. [cloudflare's ECDSA introduction](https://blog.cloudflare.com/ecdsa-the-digital-signature-algorithm-of-a-better-internet/)
7. [tls 1.2 rfc](https://tools.ietf.org/html/rfc5246#appendix-A.4.2)
8. [blog session ticket flaws](https://blog.filippo.io/we-need-to-talk-about-session-tickets/)
