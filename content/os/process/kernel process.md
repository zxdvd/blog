```metadata
tags: linux, process
```

## kernel process
Kernel also ran a lot of processes internally to do all kinds of work. They have most
 properties like ordinary process. You can see them using in `top` or `ps`. For `ps`,
 command name of kernel process is wrapped with `[]`, like `[kthreadd]`. So you can
 list all kernel processes using ` ps -ef | grep ']$'` (match the ending `[`).

Following is heading part of `ps -ef`:

```text
UID        PID  PPID  C STIME TTY          TIME CMD
root         1     0  0 Mar02 ?        00:01:05 /sbin/init noibrs
root         2     0  0 Mar02 ?        00:00:00 [kthreadd]
root         3     2  0 Mar02 ?        00:00:00 [rcu_gp]
root         4     2  0 Mar02 ?        00:00:00 [rcu_par_gp]
root         6     2  0 Mar02 ?        00:00:00 [kworker/0:0H-kblockd]
root         9     2  0 Mar02 ?        00:00:00 [mm_percpu_wq]
root        10     2  0 Mar02 ?        00:00:25 [ksoftirqd/0]
root        11     2  0 Mar02 ?        00:17:58 [rcu_sched]
root        12     2  0 Mar02 ?        00:00:05 [migration/0]
root        13     2  0 Mar02 ?        00:00:00 [cpuhp/0]
root        14     2  0 Mar02 ?        00:00:00 [cpuhp/1]
```

We know that pid 1 is the init process. It's the starter of userspace processes. Similarly,
 pid 2 (kthreadd, kernel thread daemon) is starter of kernel space processes. All other
 kernel processes are forked from it.

Attention, kthreadd is not child of init. We can see that their PPID is 0. Both them are
 started by kernel.

### kernel processes
Following are some common kernel processes:

- kswapd: it writes dirty pages to disk so that pages can be reclaimed
- migration/N: each CPU has a migration thread that rebalances workload across CPUs
- kworker/0:2-events: worker thread that handles events
- kworker/0:2-kblockd: worker thread that handles block(disk) operations
- ksoftirqd/N: handles soft irqs

### check kernel process while programming
You may need to check if a process is kernel process when writing some scripts. I found
 kubernetes using the following way to detect kernel process.

```go
// kubernetes/pkg/kubelet/cm/container_manager_linux.go

// Determines whether the specified PID is a kernel PID.
func isKernelPid(pid int) bool {
	// Kernel threads have no associated executable.
	_, err := os.Readlink(fmt.Sprintf("/proc/%d/exe", pid))
	return err != nil && os.IsNotExist(err)
}
```

For kernel process, the symbol link `/proc/<pid>/exe` exists but it links to nothing.

### references
- [redhat forum: summary of kernel threads](https://access.redhat.com/discussions/683863)
- [kernel: deferred_work](https://linux-kernel-labs.github.io/refs/heads/master/labs/deferred_work.html)
