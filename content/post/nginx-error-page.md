+++
title = "nginx: error_page"
date = "2017-01-10"
categories = ["Linux"]
tags = ["nginx"]
slug = "nginx-error-page"
Authors = "Xudong"
draft = true
+++

对于401，404之类的状态码，通常后端可以自己出来好，给出比较友好的json提示，但是500一类的就不行了，通常都是网关错误，比如后端挂了/重启/过载等等，这些状态错误一般都是nginx/apache一类web server返回的，默认一般是一个html的错误页，对api接口不太友好。

nginx提供了error_page这个配置可以对400，500的错误进行内部重定向处理。比如一个简单的配置，让500错误返回json。

``` C
  error_page 502 503 504 /50x.json;

  location /50x.json {
    return 500 '{"code": -1, "msg": "gateway error!"}';
  }
```

<!--more-->

这个就是将所有的502/503/504内部重定向(跟301/302不一样)到/50x.json，并且返回一个json字符串。这里用`50x.json`前面名字可以
随便改没关系，后面的文件后缀是有左右的，nginx默认的mime会将json后缀的请求的content-type设置成`application/json`, 这样
前端就不用再JSON.parse一下了。

这里如果不用后缀的话，也可以通过设置`default_type`来实现，如设置`default_type application/json`.

我们有的项目api是通过返回json里的code来判断请求成功和失败的，状态码全是200，那么就有类似这样的配置

``` C
  error_page 500 502 503 504 =200 @500_err;

  locaton @500_err {
    default_type application/json;
    return 200 '{"code": -1, "msg": "500 error"}';
  }
```

如果文件内容很长很大，也可以直接写在文件里，当作静态文件来serve，这样的话可以将这些400.html, 500.json, 404.xml等放入代码
库管理。

### References
1. [error_page configuration](https://gist.github.com/weapp/99049e69477f924dafa7)
2. [devdocs error_page](http://devdocs.io/nginx/http/ngx_http_core_module#error_page)
