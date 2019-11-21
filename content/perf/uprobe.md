<!---
tags: linux, perf, tracing, uprobe
-->

Kprobe helps you to trace kernel space while uprobe helps you to trace the userspace functions.
You'd better read the blog about kprobe first if you don't know it.

Kernel has addresses for all kernel symbols. It can easily get address use `kallsyms_lookup_name`.
However, it cannot do it for userspace functions. Symbol table may be stripped in the binary file 
to reduce size. Then we need to provide symbol offset for uprobe.

### a demo to trace readline of bash
We can trace readline return value of bash to monitor all bash commands.

```
root$ cd /sys/kernel/debug/tracing

# add the probe (I'll explain the `0xa7100` later)
root$ echo 'r:testu /bin/bash:0xa7100 val=+0($retval):string' >> uprobe_events
# enable it and monitor result
root$ echo 1 > events/uprobes/testu/enable
root$ cat trace_pipe

# test some commands at another console, get following result

root$ cat trace_pipe
            bash-1862  [000] d... 770527.908147: testu: (0x423555 <- 0x4a7100) val="ls"
            bash-1862  [000] d... 770537.947652: testu: (0x423555 <- 0x4a7100) val="echo 'hello world'"
```

It captures all bash readline return values tegother with pids.

### get symbol offset
The `0xa7100` is the offset of the symbol from the base load address. So how to get it?

First get symbol address from elf file. We can use `readelf` or `objdump`.

``` shell
root$ readelf  -a /bin/bash  | grep -i ' readline$'
   863: 00000000004a7100   129 FUNC    GLOBAL DEFAULT   14 readline
```

The `00000000004a7100` is the memory address of the symbol after it was loaded into memory.
Then we need to get the base address thus the first LOAD segment of ELF program headers.

``` shell
root$ readelf -l /bin/bash | grep LOAD
  LOAD           0x0000000000000000 0x0000000000400000 0x0000000000400000
  LOAD           0x0000000000100548 0x0000000000700548 0x0000000000700548
```

The virtual address of the first load the elf file is `0x0000000000400000`. So the offset is
`0x00000000004a7100 - 0x0000000000400000=0xa7100`.

For filter and fetchargs they are same as kprobe.

Brendangregg's [perf-tools](https://github.com/brendangregg/perf-tools.git) has a shell 
script `bin/uprobe` that will convert symbol to offset automatically so that you can simply 
call `bin/uprobe  'r:bash:readline cmd=+0($retval):string'` to trace readline.


### references
- [kernel doc trace uprobe](https://www.kernel.org/doc/html/latest/trace/uprobetrace.html)
- [kernel doc trace events](https://www.kernel.org/doc/Documentation/trace/events.txt)


