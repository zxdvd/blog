<!---
tags: c, basic
-->

Some people believe that C will initialize variable to zero or null. However,
 it's wrong. Many other languages (python, js, go) will do this but C don't.
For C, it only gives you a address to an allocated memory in heap or stack.

You MUST initialize it by yourself, otherwise you'll get indeterminable value.

I used to get a weird bug about socket. The guy found that the client he wrote
 need to wait 2 minutes to for the second TCP connection to same server. At
 first, he doubted about the server but finally found he got ` EADDRINUSE` while
 called the `bind()`.

It seems that nothing wrong with his code since he didn't specify the client port
 and the OS should choose a proper client port for him.

```c
    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr(client_ip);

    rc = bind(*sock,(struct sockaddr*)&client_addr,sizeof(client_addr));
```

However, kernel will choose proper port only when you set the client port as 0.
And `sin_port` of the uninialized `client_addr` is not determined. It could be
 anything.

So for this code, the port is not chosen by OS but specified by the value of
 uninialized memory. And it works for the first time. But you may get same port
 if stack frame not changed and got the `EADDRINUSE` error.

It's easy to solved this problem. Just add a memset to set it to 0.

```c
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
```

### initialization
To initialize a struct, you can use memset which will clear EVERY byte to zero.
You can also initialize when declaring.

```c
    struct sockaddr_in client_addr = {0};
```

Attention, this is different as `memset`.

- the padding bytes of struct is not initialized (if you don't use it, there is no effect)
- it will initialize float to 0.0 which is not zero bytes

### references
- [wiki: uninialized variable](https://en.wikipedia.org/wiki/Uninitialized_variable)
- [cpp reference: struct initialization](https://en.cppreference.com/w/c/language/struct_initialization)
- [stackoverflow: are members of struct in C initialized to zero](https://stackoverflow.com/questions/4080673/are-the-members-of-a-global-structs-in-c-initialized-to-zero)


