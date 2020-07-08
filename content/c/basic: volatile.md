```metadata
tags: c, memory, volatile
```

## basic: volatile
In C and C++, the volatile keyword simply indicates that you must always read the variable
 from memory and write to memory. The variable should not be cached. It may be changed at
 any time that's why it is **volatile**.


### why volatile
### cache
CPUs have caches that are much faster than memory, like L1, L2, L3 caches. It loads data
 from memory and stores at caches to improve performance. But there isn't strong consistence
 between the cache and memory. It may lead problems in concurrent programming.

### compiler optimization
The compiler will do a lot of optimization and it may reorder some instructions. For example,
 gcc will remove the whole `while` loop in below example when using `-O1` (and of course
 `-O2` and `-O3`).

```c
    int a=0;
    while(a) {
        // do something
    };
```

If you may change the `a` in another thread or a signal interrupt, you should define it as
 `volatile int a;` so that compiler knows that it must access it from memory in every loop
 and then test condition. And of course it cannot optimze the loop.

### when to use
You may need it in following scenarios:

- memory mapped IO: you can map external devices into memory and then do the IO. So you
 should not use any cache and must access memory each time you read or write.

- variable that is used in multiple threads or interrupt handler

### atomicity
Attention, the `volatile` keyword does not provide any atomic features. There is no lock
 when reading or writing it. If you need atomicity, you can use the **__atmoic macros**
 or using a explict lock.

### synchronization
`volatile` does not provide any synchronization too. If you want to know more about
 synchronization, you can read the post about [memory order](./memory model.md).

### volatile in Java
The `volatile` in Java is different from that in C. It forces program to read from or
 write to main memory directly just like C. However, it also provides extra happens-before
  relationship while C doesn't provide.

Attention that volatile doesn't provide atomicity in Java too.

#### happens-before relationship
- If thread A writes to a volatile variable X and then thread B read the X, thread B will
 see all variables that visible to thread A before A writes to X.

### references
- [wikipedia: volatile](https://en.wikipedia.org/wiki/Volatile_(computer_programming))
- [blog: java volatile](http://tutorials.jenkov.com/java-concurrency/volatile.html)
