+++
title = "C: variable number of arguments"
date = "2018-03-13"
categories = ["Linux"]
tags = ["c"]
slug = "c-variable-number-of-arguments"
Authors = "Xudong"
+++

看redis代码里碰到不少va_list相关的东西，然后系统了解并且实验了下，这里介绍下。

va_list, va_start, va_arg, va_end这几个宏是用来处理函数的变长参数的。
一个简单的例子, 下面这个sum函数的第一个参数count表示后面的参数个数，然后把后面的所有参数加起来返回，比如可以`sum(2, 10, 20);`或者`sum(3, 10, 11, 12);`

    #include <stdarg.h>

    int sum(int count, ...) {
            va_list ap;
            int i, total=0;
            va_start(ap, count);
            for (i = 0; i < count;i++) {
                    total += va_arg(ap, int);
            }
            va_end(ap);
            return total;
    }

其实对于变长参数并不一定非得前面加上个count来表示后面的参数个数，最主要的是va_arg需要知道是否参数都取完了，不能越界.
比如另外一种写法, 可以这么调用`sum(1,2,3,4, NULL);`或者`sum(10,20,30,NULL)`.
这里相当于将NULL做完一个参数结束的标示，也可以将0,-1,'!'等其他任意东西作为这个标示，只要程序内部能够合理的处理.

int sum(int n, ...) {
        va_list ap;
        int total=n;
        void *i;
        va_start(ap, n);
        while ((i = va_arg(ap, void *)) != NULL) {
                total += *((int *)&i);
        }
        va_end(ap);
        return total;
}

还有一点要说明的是，变长参数的类型是可以各不相同的，而且跟前面的一个参数也没有任何关系的.
`va_start(ap, n)`这里支持辅助定位而已. 比如下面这个例子, 可以这么调用`myprint(1, 100, "hi", 200, "hello", 300, "here", NULL);`一个int跟一个string交替，只要程序自己能够正确处理就可以.

    void myprint(int a, ...) {
            va_list ap;
            int data;
            va_start(ap, a);
            while((data = va_arg(ap, int)) != NULL) {
                    printf("num is %d\n", data);
                    printf("string is %s\n", va_arg(ap, char *));
            }
            va_end(ap);
    }

