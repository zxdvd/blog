```metadata
tags: linux, memory, oom, cgroup
```

## kernel: OOM

The word OOM (out of memory) is really clean. It means that system is out of memory.
So what causes OOM and what happens after OOM?

### causes
Memory overcommit is allowed by default and system can benefit from this most times.
Allocators expecially allocators of GCed languages like java, go, python may pre-allocate
 virtual address spaces. When applications actually write to these addresses, kernel
 will assign physical pages. And OOM happens when there isn't enough pages and swap
 is also full.

An application may also trigger OOM even though there is much memory in the system
 if it reaches its cgroup limit.

### how to deal with OOM
Kernel allows to panic when OOM happens. But this is not the default option. Kernel
 will try to find a best process to kill to get some free memory, thus's the OOM
 killer.

It will calculate score for each process and try to find the worst (the one with
 highest score) to kill. The kernel function `oom_badness` is in charge of this.

Following is the key code to get the score:

```c
/*
	 * The baseline for the badness score is the proportion of RAM that each
	 * task's rss, pagetable and swap space use.
	 */
	points = get_mm_rss(p->mm) + get_mm_counter(p->mm, MM_SWAPENTS) +
		mm_pgtables_bytes(p->mm) / PAGE_SIZE;
```

The process that uses most pages will be preferred. But sometimes, you may want to
 save your important applications that use much memory. Kernel do provide mechanics
 to adjust this. You can add a adjust score to a process.

The `/proc` system exposes some files to get and adjust score of each process. The
 `/proc/<pid>/oom_score` is the OOM score of that process.

While `/proc/<pid>/oom_score_adj` is the adjust score. You can write number into it
 to add additional score to the process. The value range is -1000 to 1000. You can
 set a negative value so that the process has less chance to be chosen while postive
 value indicates the the process is not so important and has higher chance to be chosen.

Value -1000 has a special meaning. It indicates the kernel to disable oom killing of the
 process.

There are some unkillable processes who'll not be chosen: the init process (pid 1) and
 kernel processes.

A lot of articles claimed that root processes has a 3% discount when calculating the
 badness score. But it is outdated. The commit **d46078b** removed related code so that
 root processes are treated same as ordinary processes from kernel 4.17.

### cgroup and oom
We know that OOM also happens when process reaches cgroup limit. And for cgroup OOM,
 kernel provides way to disable the OOM killer. The processes ask for memory will
 hang when OOM happens and OOM killer is disabled.

You can check whether it is disabled and whether it is under OOM using cgroup's
 `memory.oom_control` file. Like following:

```shell
 ❯ cat /sys/fs/cgroup/memory/docker/3f29131f82f8c327765f58df1f6e83ad2cb5135790fafd24b10f2afe8185b626/memory.oom_control
oom_kill_disable 1
under_oom 0
oom_kill 0
```

You can `echo 1 > /sys/fs/cgroup/memory/XXXXXX/memory.oom_control` to disable oom kill.

You can also subscribe OOM event via eventfd. You can create an eventfd and register
 it via the `cgroup.event_control` file of a memory cgroup. You'll get notified when
 OOM happens.

### references
- [lwn: taming the OOM killer](https://lwn.net/Articles/317814/)
- [kernel doc: proc filesystem](https://www.kernel.org/doc/Documentation/filesystems/proc.txt)
- [kernel doc: cgroup memory](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v1/memory.html)
