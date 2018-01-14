+++
title = "代码阅读: redis"
date = "2017-12-16"
categories = ["Linux"]
tags = ["read-code", "c", "redis"]
slug = "read-code-redis"
Authors = "Xudong"
+++

redis很久开始就看过部分，没有完整看过，现在准备好好看看记记笔记。
记笔记时没有拉起最新的代码，commitId是  9f779b33.

这次采取的策略是农村包围城市，从小的简单的依赖相对单一的着手到最终核心的部分。

for i in $(ls);do
    printf $i;
    cat $i | awk 'BEGIN{flag=1}  /^[\t ]*\/\*/{flag=0}  /\*\/[ \t]*$/ {flag=1;next} flag' |wc -l;
done
// 或者用sed  sed '/[ \t]*\/\*.*\*\//d;/[ \t]*\/\*/,/\*\//d'



### adlist.*
这是一个双向链表的数据结构，除了比我们自己写的更bugfree一点，也没什么特别的。

这里可以借此介绍一下函数指针的概念，看看`adlist.h`里的list的定义
```
typedef struct list {
    listNode *head;
    listNode *tail;
    void *(*dup)(void *ptr);
    void (*free)(void *ptr);
    int (*match)(void *ptr, void *key);
    unsigned long len;
} list;
```
这里面除了常规的head, tail, len之外还定义了三个看起来很奇怪的东西，这就函数指针，
它们本质上是指针，只不过指向了函数而已，而且这个函数的入参和返回值一并需要定义清楚，
如match，指向了一个这样的函数`int fn(void *ptr, void *key)`.

server.clients: 所有连接的客户端
server.unblocked_clients:
server.slowlog:

### endianconv.*
big endian 和 little endian转换的一些函数，这里主要是针对cluster模式，考虑到如果有
x86, arm等大小端不一样的机器混部的情况。

### notify.c
这个是当redis的数据发生改变的时候，通过这里的函数把event发生给特别的channel，利用的是
pubsub功能。
对应的文档可以看[这里](https://redis.io/topics/notifications)

### syncio.c
这里是用`while(1)`轮询的方式在NON_BLOCKING的io上来实现blocking的io.

redis默认用的都是NON_BLOCKING的io，如果没有数据就会直接返回EAGAIN或者EWOULDBLOCK, 这两个
是不同系统的定义，实际值是一样的，所以有如下代码
```
        if (nread == 0) return -1;
        if (nread == -1) {
                if (errno != EAGAIN) return -1;
        } else {...}
```

### lazyfree.c
通过名字也可以猜到，这里就是把一些需要大量用到free的地方，创建一个异步的job，在另一个
线程里处理，对普通的key的val是有判断的，比如hash或者set的长度超过64的，或者需要删除
一整个db的，这些都是异步处理的.

### blocked.c
client有block和unblock的模式，这里是设置以及一些其它处理的帮助函数.


### slowlog.*
这里是一个慢查询的记录，对每个慢查询有最大参数个数限制(in code), 最大长度限制(in code).
慢查询是以双向链表的形式存储的，有最大长度限制，超过长度了会删除旧的，具体如下
```
void slowlogPushEntryIfNeeded(robj **argv, int argc, long long duration) {
    if (server.slowlog_log_slower_than < 0) return; /* Slowlog disabled */
    if (duration >= server.slowlog_log_slower_than)
        listAddNodeHead(server.slowlog,slowlogCreateEntry(argv,argc,duration));

    /* Remove old entries if needed. */
    while (listLength(server.slowlog) > server.slowlog_max_len)
        listDelNode(server.slowlog,listLast(server.slowlog));
}
```

在slowlogInit函数里可以看到，通过listSetFreeMethod来给链表设置了一个free的函数指针，指向
的函数是slowlogFreeEntry.

最后面的slowlogCommand函数是对SLOWLOG命令的处理.

### sparkline.*
这个是基于外部的代码修改的，主要功能是用ascii码画波形图，在latency graph里有用到。

### pqsort.*
这里是从netBSD的libc库里拿出来稍加修改的一个排序算法.

### redis-check-aof.c
这是`redis-check-aof`这个命令的程序，主要就是检查和fix aof文件.


### setproctitle.c
这个是修改程序的进程名(`ps -ef`里看到的名字), bsd系的系统有setproctitle这个函数直接调用
就可以了，对于linux和macOS就需要自己写了，主要是修改**argv的内容(不能把它简单的指向别的地方),
需要注意的是不能覆盖了**argv后面的地方，后面是存环境变量的.

### lzf*
这里是压缩和解压算法.

### crc16.c, crc64.*, sha1.*
crc算法, sha1算法

### geohash_helper.*,


















































最近粗略看了[http_parser](https://github.com/nodejs/http-parser)的源代码，代码库很小，不会花太多时间，也可以再熟悉下http请求的处理，顺便写下我的读后感。

毕业后再也没有写过C了，先写下语言层面的一些东西吧。

### 语言基础
1. printf里的*占位符

    比如这段代码`printf("Url: %.*s\n", (int)length, at);`，字符串里只有一个*%*但是后面怎么有2个值要带入的? 这是因为*在某些情况下*\*也是占位符.

    `%s`可以这样写`printf("#%10.4s#\n", "hello world");`, 输入结果是`#      hell#`，10这里最少有10个字符的长度，如果不足10个字符，左边用空格补齐，4标示
最多取4个字符，所以我们只能看到`hell`这四个字符了，然后前面是6个补齐的空格。这里的10和4都是可选的配置，并且可以用`*`来表示从后面的变量里面获取。所以
`printf("#%.*s#\n", 6, "hello world");`输出的结果是`#hello #`.
<!--more-->

2. LIKELY和UNLIKELY这两个宏

    ```C
    #ifdef __GNUC__
    # define LIKELY(X) __builtin_expect(!!(X), 1)
    # define UNLIKELY(X) __builtin_expect(!!(X), 0)
    #else
    # define LIKELY(X) (X)
    # define UNLIKELY(X) (X)
    #endif
    ```
先不看上面几行，看else里面这个宏啥都没有做啊, 确实是这样的，这个宏可以让**GCC编译的时候**优化
下生成的汇编指令，告诉编译器我这个if结果为true的概率非常高(或者为false的概率非常高)，这样编译器
可以优化汇编指令，提高CPU的分支预测效率。CPU是多极流水处理的，在前一级指令比如if的结果还
没有出来前，后面的指令就开始就绪了，所以在if判断的时候它会选一个分支进入流水，如果if判断的
结果跟预测一直，非常好，后面的指令继续；如果跟预测的不一致，重新走另外一个分支继续，刚刚提前
开始的那些指令运算就浪费了。所以通过一些手段我们告诉CPU那个分支概率高可以提升整体效率。比如
我们写代码里的异常处理，我们能预判它出现的概率，如果很低，我们可以显示的告诉编译器，让它
做类似的优化。

3. 函数指针(function pointer)

    http_data_cb是一个指向一个函数的指针，被指向的函数接受3个参数返回一个int.
    ```C
    typedef int (*http_data_cb) (http_parser*, const char *at, size_t length);
    typedef int (*http_cb) (http_parser*);

    struct http_parser_settings {
      http_cb      on_message_begin;
      http_data_cb on_url;
    };
    ```

4. 宏定义 `do { XXX } while {0}`

    这个在redis里面也看了很多了，这里再写下，很多人包括我自己开始都会疑惑，不都是执行一次么，为啥要用`do while {0}`包一层，不是多此一举么？

    看这个例子
    ```C
    #define RETURN(V)                                                    \
    do {                                                                 \
      parser->state = CURRENT_STATE();                                   \
      return (V);                                                        \
    } while (0);
    ```
    假设有人这么写了一段
    ```C
    if (a)
        RETURN(0);
    RETURN(1);
    ```
    如果不用`do while (0)`包的话，上面这一段预编译展开就变成了
    ```C
    if (a)
        parser->state = CURRENT_STATE();
        return (0);
    parser->state = CURRENT_STATE();
    return (1);
    ```
    按照上面这样执行，`return (0);`在if之外，必然执行，这一段返回的永远都是0，跟原意不一样了，而用`do while (0)`包了就不会有这个问题了。

5. 宏定义里的**#**和**##**

    #是将变量值/宏字符串等转化成literal string, 如
    ```C
    #define HTTP_METHOD_MAP(XX)         \
      XX(0,  DELETE,      DELETE)       \
      XX(1,  GET,         GET)          \
      XX(2,  HEAD,        HEAD)         \

    static const char *method_strings[] =
      {
    #define XX(num, name, string) #string,
      HTTP_METHOD_MAP(XX)
    #undef XX
      };
    ```
    上面定义XX里的那个`#string`目的就是将`DELETE`变成`"DELETE"`, 最终相当于
    ```C
    static const char *method_strings[] =
      {
        "DELETE",
        "GET",
        "HEAD",
      };
    ```

    ##的作用是宏定义里的字符拼接，这个不会产品代码里的literal string, 如
    ```C
    #define MARK(FOR)                                                    \
    do {                                                                 \
      if (!FOR##_mark) {                                                 \
        FOR##_mark = p;                                                  \
      }                                                                  \
    } while (0)
    ```
    `MARK(url)`相当于下面的代码(url_mark不是字符串，而是一个变量名)
    ```C
    do {
      if (!url_mark) {
        url_mark = p;
      }
    } while (0)
    ```

6. struct packing

    ```
    struct http_parser {
      /** PRIVATE **/
      unsigned int type : 2;         /* enum http_parser_type */
      unsigned int flags : 8;        /* F_* values from 'flags' enum; semi-public */
      unsigned int state : 7;        /* enum state from http_parser.c */
      unsigned int header_state : 7; /* enum header_state from http_parser.c */
      unsigned int index : 7;        /* index into current matcher */
      unsigned int lenient_http_headers : 1;

      uint32_t nread;          /* # bytes read in various scenarios */
      uint64_t content_length; /* # bytes in body (0 if no Content-Length header) */
      ...
    };
    ```
    多数编译器都会对struct里的元素占用的内存按32bit/64bit进行对齐(也可以显示指示编译器不
对齐，有些平台必须要对齐)，这样对齐的过程中就可能有很多空隙需要padding，造成一定的浪费,
一方面我们可以适当调节它们的顺序来优化内存的布局，另一方面还可以显式设置每个元素占多少bit，
如上面的`unsigned int type : 2`, 本来是2/4字节的，但是我们可能只用到0,1,2,3这4个数，
那么2bit就可以了，不需要2/4字节了，这样节省了很多内存，其他的`flags`, `state`都是类似的。


6. 一段宏定义方式
    ```C
    /* Status Codes */
    #define HTTP_STATUS_MAP(XX)                                                 \
      XX(100, CONTINUE,                        Continue)                        \
      XX(101, SWITCHING_PROTOCOLS,             Switching Protocols)             \
      XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required) \

    enum http_status
      {
    #define XX(num, name, string) HTTP_STATUS_##name = num,
      HTTP_STATUS_MAP(XX)
    #undef XX
      };
    ```

### 代码分析

1. url_parser

    对于每个url, 将它分成schema, host, port, path, query, fragment, userinfo这7个部分，parse整个url，记录每个部分的偏移量和长度到如下struct
    ```C
    struct http_parser_url {
      uint16_t field_set;           /* Bitmask of (1 << UF_*) values */
      uint16_t port;                /* Converted UF_PORT string */

      struct {
        uint16_t off;               /* Offset into buffer in which field starts */
        uint16_t len;               /* Length of run in buffer */
      } field_data[UF_MAX];
    };
    ```
    上面这个结构并不复制记录每个部分的字符串，只是维持一个相对`*url`这个指针的起始偏移量。

2. 整个url, header的parser实际上是一个很大的状态机，所有的状态全部在`enum state`里定义
好了，我大致画一下url这块的状态流程图
   ```C
   graph TD
       A(url parser begin) -- CONNECT --> B(host s_req_server_start)
       A -- non-CONNECT --> C(path)
       C -- == * --> D(* for OPTIONS)
       C -- == / --> E(like /web s_req_path)
       C -- IS_ALPHA --> F(like http/ws s_req_schema)
       F -- == : --> G(s_req_schema_slash)
       G -- == / --> H(s_req_schema_slash_slash)
       H -- == / --> E
       H -- == ? --> I(s_req_query_string_start)
       H -- == 'at' --> J(s_req_server_with_at)
       H -- == IS_USERINFO_CHAR --> K(s_req_server)
       E -- == ? --> I
       E -- == # --> L(s_req_fragment_start)
       E -- other --> E
       I -- == # --> L
       I -- other --> M(s_req_query_string)
       L --> N(s_req_fragment)
   ```
然后如果url里有host还会再次单独parse下host部分，提取USERINFO(zxd:abc@abc.com)部分，另外
还做了一些规范检查之类的。

3. struct http_parser

    ```C
    struct http_parser {
    /** PRIVATE **/
    unsigned int type : 2;         /* enum http_parser_type */
    unsigned int flags : 8;        /* F_* values from 'flags' enum; semi-public */
    unsigned int state : 7;        /* enum state from http_parser.c */
    unsigned int header_state : 7; /* enum header_state from http_parser.c */
    unsigned int index : 7;        /* index into current matcher */
    unsigned int lenient_http_headers : 1;

    uint32_t nread;          /* # bytes read in various scenarios */
    uint64_t content_length; /* # bytes in body (0 if no Content-Length header) */

    /** READ-ONLY **/
    unsigned short http_major;
    unsigned short http_minor;
    unsigned int status_code : 16; /* responses only */
    unsigned int method : 8;       /* requests only */
    unsigned int http_errno : 7;

    /* 1 = Upgrade header was present and the parser has exited because of that. */
    /* 0 = No upgrade header present. */
    /* Should be checked when http_parser_execute() returns in addition to */
    /* error checking. */
    unsigned int upgrade : 1;

    /** PUBLIC **/
    void *data; /* A pointer to get hook to the "connection" or "socket" object */
    };
    ```

    这个就是http_parser的结构，在整个处理过程中，只记录了状态的变化，没有复制请求或者
    响应的内容之类的，内存占用确实很少，对于整个header/body的处理跟url类似，一个很大的
    状态机，所以http_parser_execute这个函数非常的大，有1400多行吧(整个http_parser.c也就2400行).

### Reference:
1. [printf asterisk(*)](https://stackoverflow.com/questions/7899119/what-does-s-mean-in-printf)
2. [function pointer](https://stackoverflow.com/questions/1591361/understanding-typedefs-for-function-pointers-in-c)
3. [struct packing](http://www.catb.org/esr/structure-packing/)
4. [struct align](https://stackoverflow.com/questions/2748995/c-struct-memory-layout)
5. [RFC 2616 Request](https://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html)
