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

4. ocstamp 


### References
1. [harden web server 2017](https://hynek.me/articles/hardening-your-web-servers-ssl-ciphers/)
2. [nginx ssl ie8](https://ablagoev.github.io/ssl/nginx/ie8/winxp/cipher/2016/12/23/ie8-winxp-nginx-ssl.html)
3. [nginx https optimizing](https://www.bjornjohansen.no/optimizing-https-nginx)
4. [IE Supported Cipher Suites](https://github.com/client9/sslassert/wiki/IE-Supported-Cipher-Suites)
5. [ssllab client and server test](https://www.ssllabs.com/projects/index.html)
6. [nginx rsa+ecdsa certificates](https://scotthelme.co.uk/hybrid-rsa-and-ecdsa-certificates-with-nginx/)
