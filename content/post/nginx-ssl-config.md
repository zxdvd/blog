+++
title = "nginx ssl config"
date = "2017-09-06"
categories = ["Linux"]
tags = ["nginx", "ssl", "https"]
slug = "nginx-ssl-config"
Authors = "Xudong"
draft = true
+++


由于运维误配置导致某些浏览器访问不了网站，然后我深入了解了下ssl相关的东西，写了一篇关于[ssl ciphers的文章]().
这一篇介绍下关于nginx ssl的一些配置的东西。

1. cipher suites
cipher suites配置会影响性能和客户端兼容性，需要介绍自己网站情况处理，如果某些平台下https无法访问，自己手机电脑访问没问题，可以
检查下是不是cipher配置问题。具体配置可以参考下[附录里第一篇文章](https://hynek.me/articles/hardening-your-web-servers-ssl-ciphers/),
讲得很详细，而且一直更新到了2017年6月。

2. 双证书
如果nginx版本在1.11以上，可以考虑用RSA和ECDSA双证书，RSA兼容性好，ECDSA证书小，性能更好，支持前向保密，这样结合起来很不错。

3. ssllab test
[ssllab](www.ssllabs.com)可以检查client和server的https支持情况，可以用这个网站检查下自己的服务器配置。

<!--more-->

4. OCSP stapling
证书签发之后，除了正常过期之外还有撤销的操作(比如私钥泄漏，域名到期等等)，那么浏览器怎么知道这个证书是否被撤销了呢？之前的老方法是
CRL，就是定时更新一个包含所以过期证书的list，这样一时效性很差，那么多CA也不可能天天挨个去更新，二是文件体积越来越大，对移动端来说
开销也很大。现在一般采用OCSP验证，证书里有CA的OCSP地址，客户端可以向这个地址请求来验证证书是否有效。
``` shell
openssl s_client -connect www.baidu.com:443  -servername baidu.com  < /dev/null | openssl x509 -noout -text | grep -C1 OCSP
```
但是这样做还是有不少问题:
1. client要发送dns查询OCSP服务器地址，三次握手建立新的连接，获取查询结果，再考虑tcp的慢启动，这样首字节延迟会非常高，影响用户体验
2. 由用户自己查询，泄漏了用户的隐私，OCSP服务器可以知道用户访问了那些网站，频率如何之类的，网站所有者也不希望这样的信息被泄漏
3. OCSP查询是http明文查询的，这样中间环节的人也可以知道用户的访问情况
4. OCSP server是一个单点，如果CA的OCSP服务器挂了，它签发的证书的网站都会受影响(let's encrypt出现过类似问题)

我感觉其中第一点是很重要的，延迟对网站来说是很重要的一个考量，解决的办法是OCSP stapling，由服务器去做这个请求，并且将这个请求缓存起来，
这样上面的问题基本都解决了，服务器可以提前请求，在缓存过期之前提前更新就可以了(这期间哪怕OCSP server挂了也没什么影响)。这样做也有
一点不好的地方，把OCSP stapling的response塞在证书里会增大了证书的大小。

另外我检查了一些网站，我发现大网站比如baidu.com, jd.com, google.com, github.com都没有使用OCSP tapling，反而一些小网站比如我自己的(
用的caddy server), stackoverflow.com之类的都有使用，我猜是这样的，大网站server端不做，让client/browser自己去做，browser自己会缓存
OCSP的结果，对于大网站，人们天天访问，首次访问没有cache的情况比较少，所以首次访问低延迟的概率是很低的，对于他们来说这是可以接受的。
对于小网站，每个终端访问频率没那么高，可能几天一次，如果browser端缓存的话，失效的概率比较高，就会经常出现首次访问低延迟的情况，所以
小网站做OCSP stapling是比较合适的。

浏览器支持: 维基百科的资料显示chrome从2012年开始不做OCSP的检查了，其他浏览器基本都是支持的。


### References
1. [harden web server 2017](https://hynek.me/articles/hardening-your-web-servers-ssl-ciphers/)
2. [nginx ssl ie8](https://ablagoev.github.io/ssl/nginx/ie8/winxp/cipher/2016/12/23/ie8-winxp-nginx-ssl.html)
3. [nginx https optimizing](https://www.bjornjohansen.no/optimizing-https-nginx)
4. [IE Supported Cipher Suites](https://github.com/client9/sslassert/wiki/IE-Supported-Cipher-Suites)
5. [ssllab client and server test](https://www.ssllabs.com/projects/index.html)
6. [nginx rsa+ecdsa certificates](https://scotthelme.co.uk/hybrid-rsa-and-ecdsa-certificates-with-nginx/)
7. [wiki OCSP](https://en.wikipedia.org/wiki/OCSP_stapling)
8. [OCSP rfc](https://www.ietf.org/rfc/rfc5019.txt)
