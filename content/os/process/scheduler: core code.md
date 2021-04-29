```metadata
tags: linux, process, scheduler
```

## scheduler: core code

The `__schedule()` function is the entry of the scheduler. It calls `pick_next_task()`
 to get a task to run and then do context switch if next task is not equal to current one.

```c
// kernel: kernel/sched/core.c
static void __sched notrace __schedule(bool preempt)
{
	struct rq *rq;

	cpu = smp_processor_id();
	rq = cpu_rq(cpu);                    /// get per-cpu runqueue
	prev = rq->curr;

	next = pick_next_task(rq, prev, &rf);    /// get a new task using all kinds of scheduler
	clear_tsk_need_resched(prev);
	clear_preempt_need_resched();
}
```

The `pick_next_task()` function tries to get next task from all the schedulers. Since
 `CFS` scheduler is the most commonly used, it checks that whether all tasks are using
 `CFS` scheduler at first. If so, there is no need to loop all scheduler classes.

Otherwise, it loops all scheduler classes from high to low to find a task.

```c
// kernel: kernel/sched/core.c
static inline struct task_struct *
pick_next_task(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
	const struct sched_class *class;
	struct task_struct *p;

	/*
	 * Optimization: we know that if all tasks are in the fair class we can
	 * call that function directly, but only if the @prev task wasn't of a
	 * higher scheduling class, because otherwise those lose the
	 * opportunity to pull in more work from other CPUs.
	 */
	if (likely(prev->sched_class <= &fair_sched_class &&
		   rq->nr_running == rq->cfs.h_nr_running)) {

		p = pick_next_task_fair(rq, prev, rf);
		if (unlikely(p == RETRY_TASK))
			goto restart;

		/* Assumes fair_sched_class->next == idle_sched_class */
		if (!p) {
			put_prev_task(rq, prev);
			p = pick_next_task_idle(rq);
		}

		return p;
	}

restart:
	put_prev_task_balance(rq, prev, rf);

	for_each_class(class) {           /// loop all classes from high to low
		p = class->pick_next_task(rq);
		if (p)
			return p;
	}
}
```

The order if scheduler class is very important. The dealine class has higher priority
 than the realtime class. And realtime class is higher than CFS class.

```c
// kernel: include/asm-generic/vmlinux.lds.h
/*
 * The order of the sched class addresses are important, as they are
 * used to determine the order of the priority of each sched class in
 * relation to each other.
 */
#define SCHED_DATA				\
	STRUCT_ALIGN();				\
	__begin_sched_classes = .;		\
	*(__idle_sched_class)			\
	*(__fair_sched_class)			\
	*(__rt_sched_class)			\
	*(__dl_sched_class)			\
	*(__stop_sched_class)			\
	__end_sched_classes = .;
```

Each scheduler class is a collection of function pointers. Following is a few members
 of it.

```c
struct sched_class {
	void (*enqueue_task) (struct rq *rq, struct task_struct *p, int flags);
	void (*dequeue_task) (struct rq *rq, struct task_struct *p, int flags);
	void (*yield_task)   (struct rq *rq);
	struct task_struct *(*pick_next_task)(struct rq *rq);
}
```

### references
- [man page: sched](https://man7.org/linux/man-pages/man7/sched.7.html)
