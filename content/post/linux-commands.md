+++
title = "linux commands"
date = "2017-09-15"
categories = ["Linux"]
tags = ["nginx", "command"]
slug = "linux-commands"
Authors = "Xudong"
draft = true
+++

一些常用的不常用的命令。

1. watch
定时执行命令并刷新输出，默认似乎是2s，如`watch 'pm2 list'`, `watch -n 3 free -m`.

### 进程相关的
1. 查看文件/目录/端口/socket被那个进程占用了
`fuser -v FILE/DIR`
`fuser -v -n tcp 80`
`fuser -v -n 8080/udp`

### 网络相关的
1. 查看一个端口被那个程序给占用了
```
# fuser  -v -n tcp 80
                     USER        PID ACCESS COMMAND
80/tcp:              root        985 F.... nginx
                     www-data   2945 F.... nginx
                     www-data   2946 F.... nginx
                     www-data   2947 F.... nginx
```

### 文件相关的
1. 批量重命名
`rename 's/\.js$/.js.bak/' *.js`

2. 获取文件的完整路径
`readlink -f FILENAME_OR_DIR`

3. find命令
3.1 查找home目录大小超过100M的文件
`find ~ -size +100M`

4. nl命令,给输出添加行号
`ls |nl`

5. inotifywait 监测文件变化
while inotifywait -e close_write *.js
        do make
done

6. 随机重排文件里的每一行
`shuf file.txt`

7. ssh断开后保持文件连接
`nohup ./script.sh &`
对于前台任务，可以放入后台，然后`disown -h jobid`

8. 限制一个命令的执行时间
`timeout 10s ./script.sh`
`while true; do timeout 30m ./script.sh;done` 每隔30m重启一次

9. 分割大文件
split

10. $SECONDS 变量，当前shell启动运行的时间

### References
1. [30 interesting commands](https://www.lopezferrando.com/30-interesting-shell-commands/)
