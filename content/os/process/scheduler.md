```metadata
tags: linux, process, scheduler
```

## scheduler

For a general OS, there are too many tasks to run. But the CPUs is limited. OS needs
 to schedule properly to satisfy different requirements.

Some want to interact smoothly. Some want to be processed immediately. Some have a
 deadline. Linux kernel supports multiple scheduling classes and scheduling policies
 to deal with these situations.

You may think that a system can configure to choose only one scheduling class. However,
 multiple scheduling classes are used at same time. And they are looped one by one to
 find a suitable task.

Currently, there are CFS, realtime and deadline scheduling classes. CFS is the default
 one since 2.6.23.

The deadline scheduling class has the highest priority and then is the realtime. And CFS
 is the lowest one. The kernel always chooses task from deadline at first. If not found,
  it will try realtime runqueue and then CFS runqueue.

### CFS
CFS (complete fair scheduler) tries best to be fair. It is the default scheduler since 2.6.23.

CFS will compute a target latency that each runnable task will run at least once in this
 latency. Ideally, each task will run period of `latency / N`.

But sometimes you way want to give some tasks more time to run. You can control this by giving
 nice value. Nice value is converted to weight. And then CFS tres to compute time slice for
 each task as `(weight_of_current_task / total_weight) * latency`.

CFS used to be fair among all threads. But it changes to be fair among groups later.

CFS supports 3 policies: SCHED_OTHER (or SCHED_NORMAL), SCHED_IDLE, SCHED_BATCH.

#### nice value
You can use `nice` value to influence task scheduling in CFS. The range is -20 to 19.
 The nice is converted to priority. The range 0-99 is for realtime scheduler. For CFS,
 the range is 100-139. By default, it is 120 (nice=0). When you set nice as -20, the
  priority is 100 (highest for CFS). When you set nice as 19, the priority is 139.

The nice/priority is convert to weight and the weight is used to calculate how much time
 slice should assign for a task.

The nice to weight table is defined in sched_prio_to_weight[40] at `kernel/sched/core.c`.
Weight of nice -20 is 88761, -1 is 1277, 0 is 1024, 1 is 820, 19 is 15. So that we can
 find that weight of highest priority task (nice -20) is more than 5900 times of lowest
 priority task (nice 19).

#### autogroup
For the CFS, it may lead to problem that a process with many threads occupies most of
 CPU times. For example, suppose you are browsing online, listening music and building
 kernel in the terminal using `make -j 16`. Then the `make` will get most of CPU since
 it has much more threads than other applications. It leads to bad experience for other
 applications.

Since kernel 2.6.38, kernel provides the autogrouping feature so that cpu time share is
 divided among autogroups but not threads. Then for above situation, the `make` can only
 get at most `1/3` CPU time.

### SMP
For a machine with multiple core CPUs, it can run multiple tasks at the same time. If all
 CPUs uses a single runqueue, then any operation on the runqueue needs lock. This will
 waste lots of resource.

A simple solution is each CPU maitains its own runqueue. But there comes following problems

- one CPU may have many tasks, another CPU may keep in idle state
- needs to balance tasks between these CPUs
- task may jump between CPUs frequently

### realtime scheduler
The realtime scheduler is used for areas that tasks need to response immediately, mostly
 industrial control area.

Kernel supports it by assign priority from 0 to 99 for each task. And high priority task (0)
 is scheduled at first absulutely.

It supports FIFO or RR (round robin) policy.

### O(1) scheduler
Used in old kernel (earlier 2.6). It has 140 priorities. Each priority has a active queue
 and expired queue. Task moves to expired queue when it uses out its time slice. And the two
 queues are swapped when the active queue became empty.

It uses bitmap to mark whether a specific priority has runnable task or not. It's O(1) to
 find the highest priority that has runnable task. That's why it is called O(1) scheduler.

### references
- [man page: sched](https://man7.org/linux/man-pages/man7/sched.7.html)
- [kernel doc: sched design CFS](https://www.kernel.org/doc/html/latest/scheduler/sched-design-CFS.html)
- [kernel: scheduler](https://helix979.github.io/jkoo/post/os-scheduler/)
- [complete guide to linux process scheduling](https://trepo.tuni.fi/bitstream/handle/10024/96864/GRADU-1428493916.pdf)
