```metadata
tags: linux, process, scheduler, stats
```

## scheduler: stats

You can get parameters and statistics of scheduler from `/proc/sys/kernel/sched_*`.
For individual task, you can get information from `/proc/pid/sched`.

###  /proc/sys/kernel/sched_*

#### /proc/sys/kernel/sched_latency_ns
It is the intial value for scheduler period, thus a period of time that all runnable
 tasks will run for at least once.

Ideally, each task will get sched_latency_ns/N time to run if there is N runnable tasks.
But if there are too many runnable tasks, the target latency will be `N * sched_min_granularity_ns`.

You may find that this value varies on different machines. Actually, it is calculated
 as `6ms * (1 + ilog(ncpus))`. For example, a machine with 4 CPUs will get 18ms.


#### /proc/sys/kernel/sched_min_granularity_ns
If there are too many tasks to run, `sched_latency_ns/N` can be very small that each
 task will run very short and then be preempted. Considering that context switch also
 uses a few cycles, then too much time is wasted on scheduling.

`sched_min_granularity_ns` is used to avoid this situation. Each task will run for at
 least `sched_min_granularity_ns` time when `sched_latency_ns/N` is less than it.

### /proc/pid/sched
This proc file is used to expose parameters and statistics of a specific task.
Following is a example of a postgres process.

```text
postgres (13494, #threads: 1)
-------------------------------------------------------------------
se.exec_start                                :   59921488769.812616
se.vruntime                                  :     195602057.695564
se.sum_exec_runtime                          :       1317539.880574
se.statistics.sum_sleep_runtime              :   13033037074.286326
se.statistics.wait_start                     :             0.000000
se.statistics.sleep_start                    :   59921488769.812616
se.statistics.block_start                    :             0.000000
se.statistics.sleep_max                      :         60080.268205
se.statistics.block_max                      :          3551.861583
se.statistics.exec_max                       :            35.072740
se.statistics.slice_max                      :             4.391082
se.statistics.wait_max                       :           324.892720
se.statistics.wait_sum                       :        295944.778290
se.statistics.wait_count                     :              1938202
se.statistics.iowait_sum                     :          7968.403012
se.statistics.iowait_count                   :                  314
se.nr_migrations                             :               381434
se.statistics.nr_migrations_cold             :                    0
se.statistics.nr_failed_migrations_affine    :                    0
se.statistics.nr_failed_migrations_running   :                29469
se.statistics.nr_failed_migrations_hot       :                 4936
se.statistics.nr_forced_migrations           :                  166
se.statistics.nr_wakeups                     :              1868905
se.statistics.nr_wakeups_sync                :               158520
se.statistics.nr_wakeups_migrate             :               370117
se.statistics.nr_wakeups_local               :                78450
se.statistics.nr_wakeups_remote              :              1790455
se.statistics.nr_wakeups_affine              :               630696
se.statistics.nr_wakeups_affine_attempts     :              1596181
se.statistics.nr_wakeups_passive             :                    0
se.statistics.nr_wakeups_idle                :                    0
avg_atom                                     :             0.683766
avg_per_cpu                                  :             3.454175
nr_switches                                  :              1926886
nr_voluntary_switches                        :              1858208
nr_involuntary_switches                      :                68678
se.load.weight                               :                 1024
se.avg.load_sum                              :               926390
se.avg.util_sum                              :               926390
se.avg.load_avg                              :                   16
se.avg.util_avg                              :                   16
se.avg.last_update_time                      :    59921488769812616
policy                                       :                    0
prio                                         :                  120
clock-delta                                  :                  162
mm->numa_scan_seq                            :                    0
numa_pages_migrated                          :                    0
numa_preferred_nid                           :                   -1
total_numa_faults                            :                    0
current_node=0, numa_group_id=0
numa_faults node=0 task_private=0 task_shared=0 group_private=0 group_shared=0
```

The `prio` is priority of the task. 120 means that it uses the default nice value (nice 0).
 The `se.load.weight` is related weight of the priority. For priority 120, it is 1024.

`nr_switches` is count of total context switch. `nr_voluntary_switches` means it is
 blocked so that scheduler picks another task to run. `nr_involuntary_switches` means
 that it is preempted by others.

`se.sum_exec_runtime` is the total CPU time in millisecond (they are tracked using nanosecond
 internally) used by the task.


`clock-delta` is difference of two `cpu_clock()`.

```c
		t0 = cpu_clock(this_cpu);
		t1 = cpu_clock(this_cpu);
		__PS("clock-delta", t1-t0);
```
