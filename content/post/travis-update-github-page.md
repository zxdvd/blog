+++
title = "配置travis自动更新github page"
date = "2016-11-05"
categories = ["Linux"]
tags = ["ci"]
slug = "travis-update-github-page"
Authors = "Xudong"
+++

很久没有更新了，evernote里倒是攒了一堆素材，就是没整理，现在准备抽空更新了。
之前是用pelican生成网页的，后来有次pelican更新了，把一个组件都弄到一个别的repo了，
然后每次配置又得多clone几个repo了。正好最近学习go，hugo这种single binary感觉也不错，
配合travis自动更新都不需要安装什么依赖的，然后就决定迁移过来了。

travis是一个自动化的编译、打包、测试、部署平台，对开源项目免费，支持n多语言，通过自定义脚本，
感觉就是送了一个打包测试的服务器，不知道他的盈利模式是什么用的，靠少量付费用户能撑起这么多
开源项目的免费使用么？

他的工作流程大致如下:

```
push OR pull request to a repo ->
    github notify this event to travis ->
        travis clone your code and build according the configuration.
```

<!--more-->

以我的github page为例，首先注册travis账号(可以直接用github Oauth登录注册)，登录后同步账号
选择需要用travis来build的repo。如果是github账号登录的，我发现它以及给所选的repo添加了webhook了
(当github收到push，pull request等消息的时候，github可以给webhook里配置的外部服务发送消息
通知)。

然后在repo的根目录里添加`.travis.yml`文件，travis会根据这个文件的配置来build，比如一个简单
的hello world
``` yaml
install: true
script:
    - echo "hello world!"
```
当把这个文件提交并推送到github后，可以发现travis那边就开始build了，而且在log里可以看到整个
的build过程，不出意外可以看到打印出的`hello world!`。

如果build的过程需要很多外部工具的支持的话，可以在install里面写，比如
``` yaml
before_install:
    - sudo apt-get install python -y
install:
    - pip install -r requirements.txt
```

其实只要恰当的设置好了language，这个build的container里面就自动包含了这个语言需要的一些基本
工具，很多时候我们需要安装的可能只是一些pip包或者npm包而已。

以我的`.travis.yml`为例
``` yaml
branches:
  only:
    - content

install: true
before_script:
    - >-
        echo -e $DEPLOY_KEY > $HOME/gh_key \
            && chmod 600 $HOME/gh_key \
            && printf "%s\n" \
                "Host github.com" \
                "    IdentityFile $HOME/gh_key" \
                "    LogLevel ERROR" >> $HOME/.ssh/config
    - git config --global user.email $GH_EMAIL
    - git config --global user.name $GH_USER
script:
    - >-
        ./bin/hugo_linux --theme=blackburn \
            && cd public \
            && git init \
            && git remote add origin git@github.com:zxdvd/zxdvd.github.io.git \
            && git add --all \
            && git commit -m 'update pages via travis' \
            && git push -f origin master
```
1. 通过branch设置可以选择只build那些branch或不build那些branch，我的博客原文在content分支，
所有只需要build content分支就可以。

2. 因为我把hugo的可执行文件也放进repo了，所有我不需要安装任何依赖，直接设置`install: true`.

3. before_script里是设置github的deploy key以及用户名配置

4. script里是hugo生成静态文件以及push到master分支的设置。

5. 整个配置中用到了3个环境变量(DEPLOY_KEY, GH_USER, GH_EMAIL)，是在travis的网页端设置的，
通过环境变量可以把一些不能直接写到`.travis.yml`的东西(比如密码，sshkey)隐藏起来。

在上面的配置中最后面有一个git push的动作，可以理解为部署，不管是部署到github的某个分支还是部署到
私人的服务器，都是需要认证的，显然认证需要的密码，秘钥是不能直接写在配置文件里的，这些都应该通过
环境变量的方式传递。使用这种方式的时候，建议关闭travis的`pull request`build的功能(别人提了
一个pull request，travis自动build下看能不能通过单元测试)，因为恶意攻击者可以篡改`.travis.yml`
文件轻易的窃取环境变量。

在上面的配置中，我使用的是repo deploy key的方式认证，github支持密码、Personal access tokens、
ssh key等多种方式认证，前两种权限太高，对全部repo都有效，而deploy key每个repo都可以设置一个或者
多个，很适合travis部署的情况。

使用步骤:
1. 生成ssh key
  >ssh-keygen -f github_page_deploy_key
2. 将`github_page_deploy_key.pub`文件内容复制到需要部署的github repo的deploy key里
3. 将`github_page_deploy_key`(这个是私钥)添加到travis的ENV里

  1. 不要直接`cat github_page_deploy_key`，这样会丢失私钥中的换行符，可以这样获取
    >python -c 'print(repr(open("github_page_deploy_key").read()))'
  2. 在travis里添加环境变量，需要自己加引号，我试了很多次才发现，所有上面的key不能少了前后的引号

4. before_script里写好一个ssh config就可以了，需要注意的是将上面的变量里的字符串恢复到文件需要使用
`echo -e`将\n转义。

至此，整个搭建过程都完成了，以后只管写markdown文档，推送后travis会自动帮忙build整个网站并且自动部署啦。

##＃ Reference:

1. [travis script](https://github.com/alrra/travis-scripts/blob/master/doc/github-deploy-keys.md)
