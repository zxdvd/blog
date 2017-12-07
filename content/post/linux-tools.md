+++
title = "linux tools"
date = "2017-09-17"
categories = ["Linux"]
tags = ["nginx", "command"]
slug = "linux-tools"
Authors = "Xudong"
draft = true
+++

一些我觉得很不错的工具.

### 特别常用
1. [ag](https://github.com/ggreer/the_silver_searcher)
文件内容搜索的，快，简单好用. 也可以pipe结果搜索

2. [fzf](https://github.com/junegunn/fzf)
shell里文件名fuzzy search，vim里也可以用，配合tmux效果更好。

3. 编辑器我一般用vim，如果不习惯vim/emacs, 可以试试[micro](https://github.com/zyedidia/micro), 我觉得掌握一个命令行
下的编辑工具是必要的. GUI界面的我觉得vscode, atom都不错.

4. [httpie](https://github.com/jakubroztocil/httpie)
命令行发送http请求，比curl方便一些.

5. unrar
mac下解压rar文件，`unrar x xxx.rar`.

### 帮助文档类
1. [devdocs.io](http://devdocs.io)
包含n多语言/数据库/框架的官方文档，关键是可以离线缓冲，查询非常快.

2. [tldr](https://github.com/tldr-pages/tldr)
man page是不是很难看懂？试试`tldr tar`, 包含很多常用命令的简介文档，主要是有examples, 非常易懂.

3. [cheat.sh](http://cheat.sh/)
感觉是个网页版的tldr, `curl cheat.sh/ps`试试, `curl cheat.sh/go/Arrays`.


### 数据库
1. [pgcli](https://github.com/dbcli/pgcli)
postgresql的命令行客户端工具，基本可以替代psql了，自动补全的功能特别好. 类似的mysql的有[mycli](https://github.com/dbcli/mycli)
