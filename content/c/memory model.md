```metadata
tags: c, memory, memory model
```

## C memory model
We wrote code sequentially and we expected that they are executed sequentially. However,
 not only may compiler reorder the instructions but also the hardware. Most hardware only
 has weak memory model. You may not aware of it when just using single thread. But you
 need to take care of it when writing concurrent multi-threads code.

Atomic variables are frequently used in multi-threads applications.

```c++
    x.store(10);
    z = y.load();
```

Actually, these atomic functions support a optional parameter that specifies the memory
 model of itself.

Currently, there are following memory models:

```c++
typedef enum memory_order {
    memory_order_relaxed,
    memory_order_consume,
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst
} memory_order;
```

### sequential consistence
By default, it is `memory_order_seq_cst` when you don't specify it. It means
 `sequentially consistent ordering` and it is the strictest ordering so that most of
 time you should use this if you're not so familiar about memory model and hardware
 architecture.

With this model, if you can see a atomic variable shared between multiple thread, then
 you can also see all other operations in other threads that happened before it even
 there is no dependency.

Sequential consistence provides strong memory order but the trade off is performance.
It needs memory fence or lock to guarantee the consistence.

### relaxed model
Contrary to the `memory_order_seq_cst`, `memory_order_relaxed` is the most relaxed
 model. Actually, there is not synchronization and it doesn't guarantee any happened
 before relationship.

```c++
// copied from https://en.cppreference.com/w/cpp/atomic/memory_order
// Thread 1:
r1 = y.load(std::memory_order_relaxed); // A
x.store(r1, std::memory_order_relaxed); // B
// Thread 2:
r2 = x.load(std::memory_order_relaxed); // C
y.store(42, std::memory_order_relaxed); // D
```

Above is allowed to produce r1 == r2 == 42 since D may happen before C due to compiler
 reordering or hardware reordering.

The relaxed model only provides atomicity but not consistence. You can use it as counter
 if there is no synchronic requirement.

### acquire/release model
Let's see an example from GCC doc:

```c++
//  Thread 1
y.store (20, memory_order_release);
// Thread 2
x.store (10, memory_order_release);
// Thread 3
assert (y.load (memory_order_acquire) == 20 && x.load (memory_order_acquire) == 0);
// Thread 4
assert (y.load (memory_order_acquire) == 0 && x.load (memory_order_acquire) == 10);
```

Both assert can pass since there is not consistent memory order of independent x and y.
Thread 3 may sees y before x while thread 4 sees x before y.

This is not possible if the memory model is sequential consistence. If thread 3 passed,
 then it is determined that y before x and this is consistent across all thread then
 thread 4 will fail.

### consume model
The `memory_order_consume` model is little more relaxed than the `acquire/release` model
 since it only guarantee synchronization of dependent variables.

Following is a simple example from GCC doc:

```c++
// Thread 1
n = 1;
m = 1;
p.store (&n, memory_order_release);             // p depends on n
// Thread 2
t = p.load (memory_order_acquire);
assert( *t == 1 && m == 1 );
// Thread 3
t = p.load (memory_order_consume);
assert( *t == 1 && m == 1 );
```

The assert in thread 2 will pass since store to m happens before store to p in thread 1.
The assert in thread 3 may fail since there is no dependeny between m and p.

### mixed model
You can use mixed model, for example, store with relaxed, load in other threads using
acquire and sequential consistence. It is supported but not suggested.

### C11 example
Most examples in this article are written in c++. For c, it has same memory models.
Following is a C11 example, you can use `xxx_explicit` to specify the memory model, the
 default is sequential consistence, same as c++.

```c
#include <stdatomic.h>

r1 = atomic_load(x);
r2 = atomic_load_explicit(y, memory_order_relaxed);
```

### real world example from redis
Redis has some internal counters and in its `atomicvar.h` it provides atomic helpers.
It tries to use the gcc atomic builtin underground and fallbacks to __sync macros and
 mutex if previous one is not available.

It uses the relaxed memory model to implement the `atomicIncr` to get better performance.

```c
#define atomicIncr(var,count) __atomic_add_fetch(&var,(count),__ATOMIC_RELAXED)
```

The gcc __atomic macros are similar to the C11 memory model that implemented in earlier
 gcc version that C11 was not supported.

Then it uses counter for memory used and lazyfree objects.

```c
// src/zmalloc.c
    atomicIncr(used_memory,__n); \
```

### references
- [gcc doc: gcc memory model](https://gcc.gnu.org/wiki/Atomic/GCCMM/AtomicSync)
- [cpp reference: std::memory_order](https://en.cppreference.com/w/cpp/atomic/memory_order)
- [cpp reference: C11 memory order](https://en.cppreference.com/w/c/atomic/memory_order)
