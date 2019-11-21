<!---
tags: linux, perf, tracing, kprobe
-->

Kprobe is a builtin tracing machanism to trace kernel events. It can inject hooks 
before of after kernel function so that you can get function parameters or return 
values.

### kernel symbols
You can get a list of kernel symbols via `cat /proc/kallsyms`. The three columns 
of the output are symbol address, symbol type, symbol name.

    # cat /proc/kallsyms | grep execve

### trace new processes (execve)
A simple example to trace new processes (execsnoop). A new process execution often 
came as `execve` syscall after `fork` syscall. Let's trace the `sys_execve` syscall.

First change workdir to `/sys/kernel/debug/tracing`, all following commands are 
executed at this directory so that no need to use long absolute paths.

``` shell
# Add a probe, `p` is enter point probe while `r` is return value probe
root$ echo 'p:test_probe sys_execve' >> kprobe_events

# check the added probes
root$ cat kprobe_events
p:kprobes/test_probe sys_execve

# then we can enable this probe
root$ echo 1 > events/kprobes/test_probe/enable

# above will only enable this one, if we have added multiple ones, we can enable all via
root$ echo 1 > events/kprobes/enable

# now we can get the trace result
root$ cat trace_pipe

# you'll get streaming events like following
root$ cat trace_pipe
           <...>-7252  [000] d... 606214.143021: test_probe: (SyS_execve+0x0/0x40)
     barad_agent-7254  [000] d... 606214.566368: test_probe: (SyS_execve+0x0/0x40)
              sh-7257  [000] d... 606214.567916: test_probe: (SyS_execve+0x0/0x40)

# you can also `cat trace`, it's a file that tracing is disabled while you are reading it

# it seems that there isn't useful information from the output. let's change the probe a little

# before replace the probe `test_probe`, we need to disable it. let's disable all
root$ echo 0 > events/kprobes/enable

# now replace it with some arguments (I'll explain it later). Then enable it
root$ echo 'p:test_probe sys_execve filename=+0(%di):string arg1=+0(+0(%si)):string arg2=+0(+8(%si)):string' > kprobe_events

# output
root$ cat trace_pipe
                  sh-9259  [000] d... 606867.566871: test_probe: (SyS_execve+0x0/0x40) filename="/bin/sh" arg1="/bin/sh" arg2="-c"
                  sh-9261  [000] d... 606867.567673: test_probe: (SyS_execve+0x0/0x40) filename="/usr/bin/awk" arg1="awk" arg2="{if($6 == "disk" && match($1,"nvme")){print $1}}"
               lsblk-9260  [000] d... 606867.572345: test_probe: (SyS_execve+0x0/0x40) filename="/bin/lsblk" arg1="lsblk" arg2="-l"

# this result is easy to guess and understand. For the last line, 9260 is the process id of this new process,
# `lsblk` is command executed, and following is arguments.
```

### fetchargs
The probe is a breakpoint, we can access memory to extract information at this point.
Kprobe has following FETCHARGS methods.

```
    %REG          : Fetch register REG
    @ADDR         : Fetch memory at ADDR (ADDR should be in kernel)
    @SYM[+|-offs] : Fetch memory at SYM +|- offs (SYM should be a data symbol)
    $stackN       : Fetch Nth entry of stack (N >= 0)
    $stack        : Fetch stack address.
    $argN         : Fetch the Nth function argument. (N >= 1) (\*1)
    $retval       : Fetch return value.(\*2)
    $comm         : Fetch current task comm.
    +|-[u]OFFS(FETCHARG) : Fetch memory at FETCHARG +|- OFFS address.(\*3)(\*4)
    \IMM          : Store an immediate value to the argument.
    NAME=FETCHARG : Set NAME as the argument name of FETCHARG.
    FETCHARG:TYPE : Set TYPE as the type of FETCHARG. Currently, basic types
                    (u8/u16/u32/u64/s8/s16/s32/s64), hexadecimal types
                    (x8/x16/x32/x64), "string", "ustring" and bitfield
                    are supported.
```

Now, let's explain the `filename=+0(%di):string arg1=+0(+0(%si)):string arg2=+0(+8(%si)):string` in above section.

From `man 2 execve` we know that first argument of execve is binary path or interpreter while second argument is 
array of string which stores all arguments of the executed binary.
So we need to get first argument of execve to get the executed binary path. And each member of second argument to 
get arguments of the executed binary. How to get it?

For x86, first 6 arguments to a function are passed via registers. They are stored at register 
`di, si, dx, cx, r8, r9`. For other architecture, you need to check related documents.
So we can get first argument via `%di` and second via `%si`. The value in register may be an integer or an address. 
If it's an address, we can get value in memory via `+0(%di)`. The 0 here is byte offset, we can change to other 
numbers like 4,8,16. The pointed value may be int8, int16, uint32 or string, we can use `TYPE` above to tell kprobe 
its type.
Then `+0(%di):string` is get first pointer argument and treat it as string. `+0(+0(%si)):string` is get value of a 
pointer defined like `char **s` while `+0(+8(%si)):string` is value of `*(*s+1)` since each pointer is 8 bytes on x86-64.

### filter
Sometimes we may want to filter the output by pid or other parameters. Kprobe do provide filters.
Suppose we want to monitor the `open` syscall of nginx. We can achieve it via following steps:

```
# add probe, first argument of sys_open is the filepath (char *)
root$ echo 'p:test_probe sys_open  filename=+0(%di):string' >> kprobe_events
# we can get processes of nginx and then add the filter
root$ echo 'common_pid == 18947 || common_pid == 18952' >> events/kprobes/filter
# then enable it and visit pages to trigger nginx opens files, the tracing result looks like following


    nginx-18952 [000] d... 696400.905463: test_probe: (SyS_open+0x0/0x20) filename="/var/www/static/js/jquery-1.11.3.min.js"
    nginx-18952 [000] d... 696400.913799: test_probe: (SyS_open+0x0/0x20) filename="/var/www/static/js/js.cookie.js"
    nginx-18952 [000] d... 696400.914235: test_probe: (SyS_open+0x0/0x20) filename="/var/www/static/css/bootstrap.min.css"
```

For number, filter supports `==, !=, <, <=, >, >=`. And for string, it supports `==, !=, ~`. The operator 
`~` is glob match. Examples

```
# to filter opened filename that match /tmp/* you can do it like following
root$ echo 'filename ~ "/tmp/*"' >> events/kprobes/filter
```

You can group several expressions using `||` or `&&`.


### brendangregg's perf-tools scripts
Brendangregg wrotes a collection of shell perf scripts that utilize kprobe/uprobe. You can 
have a try. Just [clone and run](https://github.com/brendangregg/perf-tools.git), no 
dependent packages.


### some source codes
- [kprobe user interface](https://github.com/torvalds/linux/blob/master/kernel/trace/trace_kprobe.c)
- [general probe args](https://github.com/torvalds/linux/blob/master/kernel/trace/trace_probe.c)
- [register offset table, architecture dependent](https://github.com/torvalds/linux/blob/master/arch/x86/kernel/ptrace.c)


### references
- [kernel doc trace kprobe](https://www.kernel.org/doc/html/latest/trace/kprobetrace.html)
- [kernel doc trace events](https://www.kernel.org/doc/Documentation/trace/events.txt)
