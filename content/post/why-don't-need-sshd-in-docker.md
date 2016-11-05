+++
Title = "不要在container里使用sshd的n个理由"
Date = "2014-08-19"
Categories = "翻译"
Tags = ["docker"]
Slug = "trans/why-not-sshd-docker"
Authors = "Xudong"
+++

原文链接： [why you don't need to run sshd in your docker containers](http://blog.docker.com/2014/06/why-you-dont-need-to-run-sshd-in-docker)

今天在docker官方博客上看到一篇很好的文章，很有收获，打算翻译过来。
这篇文章讲的是不在container中使用sshd的n个理由--其实我之前很喜欢在里面开个sshd方便去check状态log之类的。

在使用docker的时候，总是有人会问“怎么进入到container里面去呢？”，然
后一些人就会说“开个sshd不就行了”。当你看完这篇文章之后你会发现sshd是不
必要的，除非你需要一个ssh server的container。

一开始大家都会禁不住去用ssh server，这看起来是最简单的方法--几乎每个
人都用过ssh的。对于我们很多人来说，ssh是一个基本技能，对各种公私钥，
无密码登陆，端口转发等等都很熟悉。有了这些基础，自然而然就会想通过sshd
进入container内部了。

现在假设你要redis server或者java webservice的docker image。考虑下下面几个问题：

* **你需要sshd来干嘛？**多数情况下你可能只是需要做下备份，查看下日志，或者重启下进程、调整下配置文件，甚至用gdb、strace来调试等等。那么后面我来告诉你怎么不使用sshd来完成这些任务。

<!--more-->

* **你怎么管理这些sshd的公私钥和密码呢？**集成到image或者放到一个单独的volume？那么需要更新公私钥或者密码时怎么办呢？如果是集成到image里面的，你需要重新生成image，然后再一次部署，然后重启container，这看起来没什么大不了，不过也不简单吧。更好一点的办法是把这些东西放到一个单独的volume，不过这个方法也有明显的缺陷。你得确保container没有这个volume的写权限（这里不是太明白为啥），否则它会弄乱你的公私钥证书之类的（然后你就登不进去了）。如果是多个container共享一个这样的volume的话情况会更糟糕。如果ssh能放在别的什么地方不就会少一个大烦恼么？

* **怎么处理ssh安全相关的升级呢？**ssh server的确是很安全的，但还是会出现
安全漏洞，然后为了打补丁你不得不重新制作所有的image，然后重启所以container。这也意味着就算你使用一些基本不会危及到系统安全的服务比如memcached，你也不得不经常关注安全相关的更新等等，因为ssh的缘故你的container的安全风险要大多了。还是那句话，如果ssh能放在别的地方，这些安全相关的风险都会被隔离开，对吧？

* **你觉得仅仅添加一个ssh就行了么？**你还需要一个进程管理器，比如monit或者supervisor。因为docker只关注一个进程。当你需要使用多进程的时候，你需要一个进程管理器去管理其他的进程。这样的话，一个可靠简单的container变得很复杂了。比如你的应用程序退出了或者跑死了，本来你可以通过docker获得一些调试信息，现在你只能通过进程管理器去找这些信息了。

* **就算你能轻松的把ssh server加进去，但是你能处理好登录权限管理、安全相关的这些问题么？**在小公司可能还好。在一个大的公司，很可能你想把ssh server添加到container，然后是另外一个人负责管理远程登录的权限许可等等。这么一个大的公司一般都有很严格的关于远程登录的规章。考虑到这些，你还想把ssh server加到container里么?

#### 那么我该怎么办？？

1. ##### 备份数据
你的数据应该放在一个volume里。然后运行另一个container，通过`--volumes-from`参数来共享前一个container的数据。这个新的container就专门用来备份数据。这样做还有一个好处：有时候你需要安装一些工具（如s3cmd）来备份这些数据到其他的地方，你可以在这个备份专用的container里面随便倒腾了而不是在运行服务的那个container里弄。

2. ##### 查看日志
老办法，用volume！！将所有日志放在一个目录，然后把这个目录设为volume，像上面备份一样，开另外一个container来处理这些日志，你可以随意安装你顺手的工具来处理这些日志，始终保持你的服务在一个纯净的环境里运行。

3. ##### 重启服务
实际上所有服务都可以通过signal来重启。当你执行`/etc/init.d/foo
restart`或者`service foo restart`,这些都是通过发送特殊的signal给进程来实现的。你可以用这个命令来发送signal `docker
kill -s <signal>`.
有一些并不监听signal，他们通过特殊的socket来接受指令。如果是tcp
socket，可以通过网络来发送这些指令。如果是UNIX
socket，还是老办法，用volume。通过启用另一个container来共享这个包含UNIX socket的volume，你就可以通过它来实现重启服务等等。

    这看起来很复杂的样子?!
其实并不复杂。假设你的服务foo生成了一个socket文件`/var/run/foo.sock`,
需要用`fooctl restart`来重启服务。你可以在启动container的时候用`-v /var/run`(或者在Dockerfile里加一行 `VOLUME /var/run`)，然后当你需要重启这个服务的时候你可以用同一个image再启动另外一个container，使用`--volumes-from`参数来共享前一个container的 /var/run, 然后执行`fooctl restart`，这样 /var/run/foo.sock这个文件会覆写为重启服务的指令传给前一个container。

        :::sh
        # Starting the service
        CID=$(docker run -d -v /var/run fooservice)
        # Restarting the service with a sidekick container
        docker run --volumes-from $CID fooservice fooctl restart


4. ##### 修改配置文件
如果你修改的配置是需要长期使用的，你应该通过image来实现--因为当你启动一个新的container的时候你得到的还是久的配置，修改的部分会丢失掉。如果我需要在一个服务的使用期间修改配置呢，比如virtual
hosts？这种情况你还是应该使用volume。
配置文件应该放在一个volume里，通过另外一个专门处理编辑的container来修改。在这个container里面你可以使用一切你喜欢的工具:
ssh+你喜欢的编辑器，或者一个有api借口的web
service，或者一个从其他的地方来获取配置的crontab任务等等。这样的话可以隔离风险:
一个container专门来跑服务，另一个专门处理配置更新的相关事务。可是有的时候我只是需要临时改下配置，怎么办??!请看下一章节。

5. ##### 调试服务
这是我能想到的仅有的你真的需要使用container的shell的场景。你可能需要gdb、strace、修改配置等等。这种情况你需要
nsenter。

#### nsenter介绍
nsenter是一个可以进入到linux的各个namespace的工具。它可以进入现存的namespace，或者在新的namespace里运行程序。
什么是namespace？namespace是linux内核的一个特性，container技术的核心要素之一。
简单的说，有了nsenter，你可以获得任何container的shell即使这个container没有ssh一类的服务。

1. ##### 怎么安装
Github repo [jpetazzo/nsenter](https://github.com/jpetazzo/nsenter). 运行

        docker run -v /usr/local/bin:/target jpetazzo/nsenter
会安装nsenter到/usr/local/bin，
然后你就可以用它了。有一些发行版已经包含了nsenter（util-linux包）。

2. ##### 怎么使用
第一步获得container的PID

        PID=$(docker inspect --format {{.State.Pid}} <container_name_or_ID>)
然后

        nsenter --target $PID --mount --uts --ipc --net --pid
这样你就获得了这个container的shell。如果你想在脚本或者自动化环境里使用，可以将它作为参数传给nsenter。它的工作方式跟chroot非常类似。

#### 关于远程控制
如果你想远程进入一个container，至少有2种方法：

1. ssh登陆到docker host，然后使用nsenter进入这个container
2. ssh登陆到docker host，通过特殊的key来限制使用nsenter

第一种方法很简单，但是需要有docker host的root权限（从安全角度来说不太好）。
第二种方法是在ssh的authorized_keys文件加上command=字段来控制。
典型的authorized_keys文件如下

    :::sh
    ssh-rsa AAAAB3N…QOID== jpetazzo@tarrasque

你可以限制ssh使用的命令。比如你希望能远程查看你的系统的可以内存，但是你不希望把整个shell暴露出去，你可以这样修改authorized_keys文件

    :::sh
    command="free" ssh-rsa AAAAB3N…QOID== jpetazzo@tarrasque

这样，当你通过这个ssh
key登录进来，它会执行free命令而不是给你一个shell。除了执行free它啥都做不了。

#### 总结
那么真的不能在container里面使用ssh server么？实际上，这也没什么大问题。当你不能ssh到docker host却有需要进入container的时候ssh还是很方便的。
但是我们也要注意到我们有很多方法可以实现我们想要的功能并且不使用ssh server。
docker允许你使用任何你喜欢的方式。不要总是把container想成一个小的vps，它还有很多其他的方式方法帮你做一个好的决定。
