+++
title = "github笔记: 常用命令行文本处理工具"
date = "2017-10-25"
categories = ["Linux"]
tags = ["shell", "linux"]
Authors = "Xudong"
draft = true
+++

#### cat, less, tail, head
1. 例子里用的都是printf, 我了解到的跟echo的差别就是echo会在结尾加一个`\n`, printf不会

2. cat show special characters
        cat -T xx.txt       # show tab as ^I
        cat -E xx.txt       # show end as $
        cat -v xx.txt       # show non-printing except for LFD and tab
        cat -e  xx.txt      # equals to cat -vE xx.txt

#### grep
*. count number of matching lines
        grep -c 'are' poem.txt

*. limit number of matching lines
        grep -m2 'blue' xx.txt

*. match any of multiple strings
        grep -e 'blue' -e 'you' poem.txt
        grep -f strings.txt     poem.txt        # read multiple strings from file

*. match whole word
        grep -w 'are' xx.txt        # match are but not area, bare
        grep -x 'hello' xx.txt      # match line with only 'hello'

*. color
        grep --color=auto 'are' xx.txt

*. get only matching portion
        grep -o 'are*b' xx.txt

* script purporse: doesn't print message to std
        grep -q 'are' xx.txt && echo found    # exit code is 0 if matched


### References
[github repo](https://github.com/learnbyexample/Command-line-text-processing)
