+++
title = "linux cgroups practice"
date = "2017-12-02"
categories = ["Linux"]
tags = ["linux", "cgroups"]
Authors = "Xudong"
draft = true
+++

cgroups 一直处于了解一些，但是没有实操的状况(docker不算). 今天由于实际需要就多了解了并配置了下.

### 场景
同事在项目中使用了puppeteer，上线没多久，我偶然发现服务器上出现了40多个chrome进程，占用了不少资源，
虽然检查出代码中的bug并进行了修复，但是还是有些担心chrome占用太多系统资源的问题，决定用cgroups限制
下. (这个后期独立出一个服务进行隔离应该是更好的选择)


### what is cgroups
cgroups是kernel提供的功能，可以对系统资源按照cpu, memory, cpuset, devices, net等多个方面进行限制.

### prepare
1. 怎么看我的系统有这些功能支持么?
`cat /proc/cgroups`可以通过最后一列`enabled`看系统开启了那些子系统的支持。`ls /sys/fs/cgroup/` 也可以。
有时候会发现可能只支持了一部分，可以通过修改grub启动参数或者修改kernel配置并重新编译来实现。

2. 需要按照那些包呢？
功能是kernel提供的，但是需要一些用户态的工具来管理和配置。对于debian, 可以`apt-get install cgroup-tools`

### how to
配置过程分为配置分组, 配置规则，使配置生效3个步骤

#### config group
根据自己的需要来写group的配置文件，如我的`/etc/cgconfig.conf`(文件名随意)
```
group chrome {
        pids {
                pids.max = 8;
        }
        cpu {
                cpu.cfs_quota_us = 100000;
                cpu.cfs_period_us = 100000;
        }
        memory {
                memory.limit_in_bytes = "512m";
        }
}
```
这里我是新建了一个叫chrome的group，里面对pids, cpu, memory这三个子系统做了限制。
对于pids，我限制这个group最多可以有8个进程，也就是进程数到8个的时候，clone/fork之类的就会报错。
对于cpu，我限制了这个group在100ms内最多使用100ms的cpu时间，也就是我的4核cpu这个组只能用一个核。
对于memory，限制这个group最多使用512m.

这里可以建多个group，每个子系统可以有更多的配置，需要参考文档根据自己的需求来配置.

#### config rule
这里需要设置一些规则，比如哪些用户，哪些命令使用那个group的规则，如我的`/etc/cgrules.conf`文件
```
*:PATH_TO_CHROME cpu,memory,pids chrome/
```
1. 似乎必须用`/etc/cgrules.conf`这个文件，`man cgrulesengd`可以发现它就指定了这个文件，也不能通过参数改变
2. `*:PATH_TO_CHROME`前面的*表示任意用户，如果你只需要限制某些用户可以改成`@admin`, '@user1'等
3. 这里也是可以配置n多个的，而且多个配置可以归属到同一个组

#### start
两个命令，都可以运行多次
```
# cgconfigparser -l /etc/cgconfig.confg
# cgrulesengd
```
前一个命令加载创建需要的cgroup，后一个根据设置的规则去应用这些配置，运行后可以发现我需要限制的哪些程序
都已经应用这些规则了。
如果对规则有修改，可以再次运行这两个命令更新。

如果需要使cgroups每次重启都生效，可以把这两个命令写入`/etc/rc.local`文件。

#### check
我们来检查下这些设置有没有生效。
1. 我们找一个正在运行的chrome进程，如`4000`, `cat /proc/4000/cgroup`可以看到这个进程对于的cgroups
2. `cat /sys/fs/cgroup/cpu/chrome/cgroup.procs`, 可以看到chrome这个group所限制的那些进程

#### remove
创建一个sub group的时候我们可以通过`/etc/cgconfig.config`或者直接`mkdir /sys/fs/cgroup/pids/XXX`这样的
方式建一个`XXX`的cgroup，但是删除的时候并不是把配置里删了或者删除目录就可以(实际上目录直接也删不了), 需要
通过`cgdelete`来删除，比如`cgdelete pids:chrome`删除pids下面的chrome这个group.

### References
1. [digitalocean: using cgroups on centOS 6](https://www.digitalocean.com/community/tutorials/how-to-limit-resources-using-cgroups-on-centos-6)
2. [linux inside: cgroups](https://0xax.gitbooks.io/linux-insides/content/Cgroups/cgroups1.html)
3. [cgroups description](https://gist.github.com/juanje/9861623)
