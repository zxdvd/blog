+++
title = "Linux command netcat"
date = "2014-07-21"
categories = "Linux"
tags = ["linux"]
slug = "linux-nc"
Authors = "Xudong"
draft = true
+++

Nc (netcat) is a very easy and useful tool that almost every distributions ship
 with. You can use it to read or write data from/to network. But before I gave
 detailed examples you need to take care of following two things:

* In some distribution it's nc while in another it's netcat. They are the same
things.
* It seems that there are two versions of netcat--openbsd version and traditional
 version. And it is said that openbsd version has more features like IPv6 support.
  And at least there is a big difference--You need a "-p" before the port number
 in traditional version.

e.g. `nc -l 8000` (openbsd)   ==   `nc -l -p 8000` (traditional)

Here are some examples of netcat.
### 1. Port scanning
You can use netcat to scanning openning ports of a server.

``` shell
    $nc -vz 192.168.1.1 1-1024
    Connection to 147.2.207.1 80 port [tcp/http] succeeded!
    nc: connect to 147.2.207.1 port 81 (tcp) failed: Connection refused
    nc: connect to 147.2.207.1 port 82 (tcp) failed: Connection refused
```

option v means verbose while option z means "use zero IO".

<!--more-->

### 2. File server
I use netcat to transfer files frequently. I think it's more handy than
"python3 -m http.server".

1. Assume there is a computer A with IP 192.168.1.10 and a computer B with IP
192.168.2.10. Now let's create a server and client to play.

        A-computer$ nc -l 8000
    This will start a tcp server at computer A. It'll listen port on 8000 (you
    need root privilege if you want to try port 1-1023).

    Then you can type some stings like "hello world netcat". They'll be buffered
     and sent out once connection is established.

    Now let's use computer B to connect to it.

        B-computer$ nc 192.168.1.10 8000
    You'll get "hello world netcat" immediately. And you can type string in either
     server side or client side. Another side will receive it.

2. Now it really looks like a chat server. But it can do more.
``` shell
        A-computer$ nc -l 8010 < /etc/hosts              //start a server and buffer a file to send
        B-computer$ nc 192.168.1.10 8010 > /tmp/hosts    //connect to the server and write received to a file
```
    OR
``` shell
        A-computer$ nc -l 8020 > /tmp/abc.tar.gz   //start and wait for connection then write received to a file
        B-computer$ cat /tmp/test.tar.gz | nc 192.168.1.10 8020  //connect to the server and send the file
    *Note: "cat FILE | nc IP PORT" is same to "nc IP PORT < FILE"*
```
    OR
``` shell
        A-computer$ tar -cvf - /etc | nc -l 8030     //tar /etc output to stdout then pipe to nc server
        B-computer$ nc 192.168.1.10 8030 > backup_etc.tar
```
    Up to here, I think you are familiar about using nc to transfer files. Let's do more tricks.

5. Play netcat with /dev/tcp
    I often need to tranfer files from an environment which don't have netcat and python. So how?
``` shell
        A-computer$ nc -l 8040               //start a server in a computer has netcat
        B-computer$ echo "hello, I am a core box without netcat" > /dev/tcp/192.168.1.10/8040
```
    Now you can see the string echoed in computer A. And different from the above examples, the tcp server got down after it received the string because netcat is a one-shot connection.

    You can send files or strings via /dev/tcp/IP/PORT to an opened connection. You can google /dev/tcp to get more information about it.

    So of course we can use this trick to send files like:

      `B-computer$ cat /tmp/etc.tar.gz > /dev/tcp/192.168.1.10/8040`

### Remote shell
This is useful when you need to access a box without telnet or ssh.
Server side:

```
    Server$ mkfifo /tmp/fifo
    Server$ cat /tmp/fifo | /bin/bash -i 2>&1 | nc -l 8050 > /tmp/fifo
```
Then you can access this computer via another computer just like ssh.

  > Client$ nc 192.168.1.10 8050

Expanation:

You can search or "man fifo" to get more about fifo. Simply, it just like pipe.

If the client connected to server, and send a command like "hostname", the server will write it to /tmp/fifo because "nc -l 8050 > /tmp/fifo". Then "cat /tmp/fifo | /bin/bash -i 2>&1" means the command is piped to bash. The option i assures bash to work in interactive mode while "2>&1" let you get both stdout and stderr. At last the output of the command is ready for nc server to send it to the client. Then you would see it in the client and begin to put in another command.

I know the traditional netcat has a option e so that you can easily use "nc -l 8050 -e /bin/bash -i 2>&1" to start a remote shell but the above method is more general. It supports both openbsd and traditional netcat though a little complex.
