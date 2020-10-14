```metadata
tags: c, memory, sanitizer
```

## address sanitizer
Address sanitizer can help you find lots of memory problems on runtime. Both clang and
 gcc support it and you can enable it with flags.

It finds following problems:

    Use after free (dangling pointer dereference)
    Heap buffer overflow
    Stack buffer overflow
    Global buffer overflow
    Use after return
    Use after scope
    Initialization order bugs
    Memory leaks

You can read google sanitizers wiki to get detail about how it works. It replaces `malloc`
 and `free` and addes some small tricks.

It divides virtual memory to too disjoint classes: `Mem` and `Shadow`. The `Mem` is used
 by user application code while the `Shadow` is a mapping of `Mem`. Each 8 bytes in `Mem`
 is mapped to 1 byte in `Shadow`. This one byte logs whether the related 8 bytes is poisoned.

It has a little CPU overhead since it will do sanitizing when reading or writing an
 pointer.

It also has memory overhead for the `Shadow` memory used to record application memory
 states.

### references
- [google sanitizers: introduction](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- [google sanitizers: AddressSanitizerAlgorithm](https://github.com/google/sanitizers/wiki/AddressSanitizerAlgorithm)
- [gcc: command options](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html)
- [gcc source: sanitizer](https://github.com/gcc-mirror/gcc/tree/master/libsanitizer)
