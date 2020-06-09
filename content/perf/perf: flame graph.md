<!---
tags: linux, perf, flame graph
-->

We can use perf to sample CPU events and generate flame graph so that we can find what
 the CPU is busy doing.

You can sample whole system or a specific process.

```shell
-- to sample whole system for 10 seconds using 99HZ sample ratio
# perf record  -F 99  -ag -- sleep 10
-- to sample a specific process (pid 6666) for 5 seconds
# perf record  -F 99  -p 6666 -ag -- sleep 5
```

You'll get a `perf.data` file. You can use `pert report` to view the report. It will
 group by process and order by percent.

```
Samples: 15K of event 'cpu-clock', Event count (approx.): 159828281230
  Children      Self  Command          Shared Object                 Symbol
+   45.31%     0.01%  swapper          [kernel.vmlinux]              [k] cpu_startup_entry
+   45.16%     0.01%  swapper          [kernel.vmlinux]              [k] default_idle_call
+   45.16%     0.00%  swapper          [kernel.vmlinux]              [k] default_idle
+   45.16%     0.00%  swapper          [kernel.vmlinux]              [k] arch_cpu_idle
+   44.80%    44.80%  swapper          [kernel.vmlinux]              [k] native_safe_halt
+   41.83%     0.00%  swapper          [kernel.vmlinux]              [k] start_secondary
+   13.09%     0.00%  postgres         postgres                      [.] 0xffffaa40e00d8338
+   13.09%     0.00%  postgres         postgres                      [.] 0xffffaa40dfe7f274
...
```

Brendan wrote some tools to generate the flame graph. You can clone it and then generate
 graphs.

```shell
# git clone https://github.com/brendangregg/FlameGraph
# perf script | PATH_TO_REPO/stackcollapse-perf.pl > out.perf-folded
# PATH_TO_REPO/flamegraph.pl out.perf-folded > perf-kernel.svg
```

Then you can download and view it in browser.

### references
- [Brendan Gress blog: cpu flame graph](http://www.brendangregg.com/FlameGraphs/cpuflamegraphs.html)
