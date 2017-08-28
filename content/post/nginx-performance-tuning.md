+++
title = "nginx: performance tuning"
date = "2017-07-27"
categories = ["Linux"]
tags = ["nginx"]
slug = "nginx-performance-tuning"
Authors = "Xudong"
draft = true
+++

下午某个时间段，系统变卡了，我自己试了下，登陆页只是一个纯静态的html也偶尔会报502，系统之前一支运行正常，似乎跟另一个团队刚部署的应用有
关系。

我检查了我们的node服务，看起来没什么异常，然后我开始看nginx的errorlog，发现了一些问题。

1. worker_connections are not enough
errorlog里面很多类似的告警，一直在刷

>
[alert] 26347#0: *69696938 2048 worker_connections are not enough while connecting to upstream, client: 116.236.XXX.XXX, server: abc.example.com, request: "GET /api/xxx"

<!--more-->

似乎worker_connections 设置成2048是不够的，我增大到9000多似乎还是不够，最后直接设置到30000了，似乎够了。这个参数是每个worker能支撑的最大连接数，如果开了4个worker，就要乘4，需要注意的是这里还包含proxy的connection，假设有2000个用户在浏览，平均每个用户6个tcp连接，然后每个连接到nginx这里还需要proxy到backend，总连接数大致是2000 * 4 * 2 = 16000.

加的配置
``` C
events {
  worker_connections 30000;
}
```

2. too many open files

>
[alert] 25039#0: *73300371 socket() failed (24: Too many open files) while connecting to upstream, client: 116.236.XXX.XXX, server: abc.example.com, request: "GET /api/xxx"

worker_connections的文档已经提到了，最大连接数不能超过线程的最大打开文件数(ulimit). 首先通过`ulimit -a`查看open files的限制，如果比较小如4096，就得把这个改大，我们系统是65535，就不用改了。

虽然系统本身的限制是没有问题的，但是每个用户还有软件本身都可以修改这个限制的，我们的nginx是以root权限启动的(master是root，然后它可以以其它用户权限启动worker), 那么很可能是nginx自己加了限制。

通过`ps -ef |grep 'nginx.*worker'`随便找一个worker，假设是24550, 如下，可以看到Max open files 是1024, 这个肯定是不够的，可以通过nginx的配置`worker_rlimit_nofile`来增大这个。

``` bash
# cat /proc/24550/limits
Limit                     Soft Limit           Hard Limit           Units
Max cpu time              unlimited            unlimited            seconds
Max file size             unlimited            unlimited            bytes
Max data size             unlimited            unlimited            bytes
Max stack size            8388608              unlimited            bytes
Max core file size        0                    unlimited            bytes
Max resident set          unlimited            unlimited            bytes
Max processes             257774               257774               processes
Max open files            1024                 4096                 files
Max locked memory         65536                65536                bytes
Max address space         unlimited            unlimited            bytes
Max file locks            unlimited            unlimited            locks
Max pending signals       257774               257774               signals
Max msgqueue size         819200               819200               bytes
Max nice priority         0                    0
Max realtime priority     0                    0
Max realtime timeout      unlimited            unlimited            us
```

配置完了之后，可以同样的方法查看新的worker的limits，看生效了没有。

``` C
worker_rlimit_nofile 30000;
```

3. nginx reload
每次修改配置可以通过`nginx -s reload`来使配置生效，但是有时候会发现修改了似乎没有生效，这个跟nginx的graceful reload有关，nginx不会直接kill旧的worker，而是新的worker处理新的请求，旧的worker还是继续处理已经保持的tcp连接，知道该worker的所有请求都关闭了，然后自动退出。

如下，旧的worker可以很清晰的看出来的
``` bash
# ps -ef |grep nginx
root      2145 27153  0 16:52 pts/4    00:00:00 grep nginx
root      2743     1  0 Jul18 ?        00:00:00 nginx: master process /usr/sbin/nginx
www-data 18718  2743  0 10:59 ?        00:00:03 nginx: worker process is shutting down
www-data 19033  2743  0 11:02 ?        00:00:02 nginx: worker process is shutting down
www-data 19034  2743  0 11:02 ?        00:00:03 nginx: worker process is shutting down
www-data 19127  2743  1 11:04 ?        00:05:44 nginx: worker process
www-data 19128  2743  1 11:04 ?        00:05:45 nginx: worker process
www-data 19129  2743  1 11:04 ?        00:05:44 nginx: worker process
```

如果希望尽快的生效，可以直接把旧的worker kill掉，但是得自己衡量下可能会带来些什么问题，比如如果用`weboscket/sse`之类的话，可能在kill的同事有大量的连接重连导致瞬时的冲击。

4. use epoll配置

很多文档都写了设置`use epoll`, 虽然官网没找到那里说了在linux上默认用了epoll(感觉这个更可能是发行版打包的配置吧), 但是至少debian jessie上不需要这么配置，默认就用了epoll(我用strace看的).

### References
1. [worker_connections](http://devdocs.io/nginx/ngx_core_module#worker_connections)
2. [worker_rlimit_nofile](http://devdocs.io/nginx/ngx_core_module#worker_rlimit_nofile)
3. [linode docs](https://www.linode.com/docs/web-servers/nginx/configure-nginx-for-optimized-performance)
4. [a good configuration with comment](https://gist.github.com/denji/8359866)
5. [connection keepalive log buffer](http://blog.martinfjordvald.com/2011/04/optimizing-nginx-for-high-traffic-loads/)
