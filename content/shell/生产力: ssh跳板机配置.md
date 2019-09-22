注意，下面所写的完全正当合规，并没有绕过跳板机，只是通过配置让它变得透明了，所以不用担心这样会降低安全性之类的问题.

跳板机，jumper server很多公司的生产环境都有使用。假设线上有app-server1,app-server2,nginx1,mysql1等机器，
为了更好的安全性，这些机器一般都不会直接暴露在公网的，那么当然就没法直接ssh登录，这个时候一般是ssh到一个jumper服务器，
然后从这个jumper上再ssh登录到需要的机器上, 这个jumper服务器就是所谓的跳板机.

这样更安全以及便于管理，但是使用上就麻烦了，每次都需要先ssh jumper，然后在jumper里面ssh xxx，这个可能还能忍受下，
但是用scp拷贝文件时就得崩溃了，需要先scp到jumper，然后jumper上继续scp.

在[熟悉了ssh config再也不用死记那么多服务器地址](https://www.toutiao.com/i6700025110645965324/)里面有如下配置

  Host k8s
      HostName 172.16.10.1
      ProxyCommand ssh jumper -W %h:%p
      User zhangsan

现在应该大致看出来了吧，这个ProxyCommand就是解决跳板机的，jumper只是跳板机的名字(这里jumper跟k8s类似，是另外一个ssh host
配置的名字，没有特别的意思)。

这样配置后我们就可以happy的ssh k8s来登录到受保护的内网机器，ssh会自动帮我们从跳板机跳过去，使用过程完全感觉不到跳板机的存在，
而且scp之类的也一样，直接scp k8s:/tmp/xx ./这样就可以.

注意，如果是使用public key免密登录，你的public key不仅需要在jumper服务器里，还需要添加在k8s这台服务器的~/.ssh/authorized_keys文件里.

放一个完整的配置如下，配置好之后ssh mysql就自动通过jumper代理过去了.

    Host jumper
        HostName ssh-jumper.baidu
        Port 2222
        User ops
    Host mysql
        HostName 192.168.10.10
        ProxyCommand ssh jumper -W %h:%p
        User root

到此这个配置基本就ok了，赶紧试试吧。
