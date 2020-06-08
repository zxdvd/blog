```metadata
tags: c, basic
```

## why extern C

the `extern "C" {...}` is very common in C source files. An example from postgres:

```c
// src/interfaces/libpq/libpq-fe.h

#ifdef __cplusplus
extern "C"
{
#endif

...

#ifdef __cplusplus
}
#endif
```

Almost the whole file is wrapped in this `extern "C"`. From the `#ifdef __cplusplus`
 we can guess that it's about c++.

Actually, it's about symbol name of object files. For C, the symbol name is just function
 name or variable name.

However, for C++, it is a little complex. C++ supports function overload, you can have
 multiple functions with same names but have different argument length or types.

```c
    void print(int a);
    void print(char *s);

    int sum(int a int b);
    float sum(float a, float b);
```

It's nice feature but then you cannot use function name as symbol name since it is not
 unique. Then there comes the name mangling rules that will encode other information
 like arguments into the name to get an unique name.

For example, on my machine, the `int sum(int a, int b)` will be resloved to `_Z3sumii`,
 `int sum(int a, int b, int c)` will be `_Z3sumiii` when using `g++`.

You can use `readelf -s ELF_FILE` to get the symbols.

```
# g++ test_export_c.c  -o cpp.out | readelf -s cpp.out | grep FUNC
     3: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND __libc_start_main@GLIBC_2.2.5 (2)
     6: 0000000000000000     0 FUNC    WEAK   DEFAULT  UND __cxa_finalize@GLIBC_2.2.5 (2)
    29: 0000000000000590     0 FUNC    LOCAL  DEFAULT   13 deregister_tm_clones
    30: 00000000000005d0     0 FUNC    LOCAL  DEFAULT   13 register_tm_clones
    31: 0000000000000620     0 FUNC    LOCAL  DEFAULT   13 __do_global_dtors_aux
    34: 0000000000000660     0 FUNC    LOCAL  DEFAULT   13 frame_dummy
    47: 0000000000000780     2 FUNC    GLOBAL DEFAULT   13 __libc_csu_fini
    48: 0000000000000560    43 FUNC    GLOBAL DEFAULT   13 _start
    51: 0000000000000784     0 FUNC    GLOBAL DEFAULT   14 _fini
    52: 0000000000000000     0 FUNC    GLOBAL DEFAULT  UND __libc_start_main@@GLIBC_
    57: 0000000000000000     0 FUNC    WEAK   DEFAULT  UND __cxa_finalize@@GLIBC_2.2
    58: 00000000000006a4    28 FUNC    GLOBAL DEFAULT   13 _Z3sumiii
    62: 0000000000000710   101 FUNC    GLOBAL DEFAULT   13 __libc_csu_init
    64: 0000000000000690    20 FUNC    GLOBAL DEFAULT   13 _Z3sumii
    67: 00000000000006c0    68 FUNC    GLOBAL DEFAULT   13 main
    68: 0000000000000520     0 FUNC    GLOBAL DEFAULT   10 _init
```

If you write programs that using pure C or pure C++, you won't get problems. But when
 you mix C with C++, or you write a library that may be used by both C and C++, name
 mangling will make troubles. If you include C header `int sum(int a, int b)` in C++,
 it will be compiled to `_Z3sumii`, linker cannot find the correct the symbol.

Then the `extern "C"` came to address this problem, it told the compiler they are
 extern C definitions and do not do name mangling with them.

Following is the an example library header from luajit:

```c
// https://github.com/LuaJIT/LuaJIT/blob/v2.1/src/lua.hpp
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "luajit.h"
}
```

You can wrap headers in a `extern "C"` in a standalone hpp header just like above.

### references
- [wikipedia: name mangling](https://en.wikipedia.org/wiki/Name_mangling)
