ssh是运维和后端程序员每天都会频繁使用的工具，登录各种测试或者生产环境的主机来查看机器状态或者调试问题等。
如果公司很小就几台机器可能还好，如果需要管理的机器比较多怎么办？像下面这样的各种username,ip,port很难记住


    ssh zhangsan@192.168.1.10
    ssh root@172.16.10.1
    ssh google@8.8.8.8


如果能够设置别名就好了，比如ssh nginx, ssh k8s-master, ssh mysql, ssh test-nginx.
有的人通过设置shell alias搞定，不过alias不能有空格，那就得是ssh-nginx这样了，
但是这样的话对于scp以及其他的依赖ssh的工具就不起作用了。有更好的方式么？

其实ssh本身就有这样的配置，以ssh zhangsan@192.168.1.10为例，
在~/.ssh/config文件里做如下配置(如果文件不存在，直接新建一个就可以)

    Host nginx
       HostName 192.168.1.10
       Port 22
       User zhangsan

然后就可以ssh nginx来替代了，因为22是默认端口，所以Port 22可以不配置. 这样配置有很多好处:
* scp也可以用，比如 scp nginx:/tmp/xx ./或者 scp ./xxx nginx:/tmp都可以.
* 虽然配置的User是zhangsan, 但是也可以用 ssh root@nginx来切换其他用户.
* 很多其他应用比如ansible也是支持的

ssh的配置项非常多，上面只是配置了最简单的三个，如果有其他需要配置的也是一样的，比如

    Host k8s
        HostName 172.16.10.1
        ForwardAgent yes
        Port 2222
        CheckHostIp no
        ProxyCommand ssh jumper -W %h:%p
        IdentityFile ~/.ssh/k8s_rsa
        User root

如果有些配置是常规的需要对所有的Host生效，可以这样配置

    Host *
        ServerAliveInterval 60

从openssh 7.3p1开始, ssh config支持**Include**指令了，如果管理的机器比较多(个人/公司/开发/测试)，
可以类似这样组织

    In ~/.ssh/config
        Include dev_config
        Include prod_config
        Include my_personal_config
        Host *
            ServerAliveInterval 60

    In ~/.ssh/dev_config
        Host dev_node1
            XXX

    In ~/.ssh/prod_config
        Host prod_k8s
            XXX

想了解更丰富的ssh的配置，可以man ssh_config查看.

按照上面的范例，我们可以配置更多的host，以后就可以ssh nginx, ssh mysql这样玩啦.

这篇是从我的头条号搬过来的，[链接地址](https://www.toutiao.com/i6700025110645965324/).
