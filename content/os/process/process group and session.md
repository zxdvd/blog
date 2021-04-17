```metadata
tags: linux, process
```

## process: group and session

Both group and session are ways to organize tasks. A process group is a collection of
 processes. And a session is a collection of process groups.

### session
You may wonder why there is session. Kernel grouping multiple processes into a session
 so that it can do some cleanup when the session is ended (like logout or ssh disconnected).

When you logged into system, or started a new shell in terminal tab, you get a new session.
 Each session may have 1 control terminal. And only one process group can control the this
 terminal at once. This group will receive user inputs or signals from the terminal.

You can get sessionid of process using `ps` like following:

    $ ps -e -o user,pid,ppid,c,stime,tty,time,sid,tty,pgid,command

The `sid` column is sessionid of process id.
'
### group
When you opened a shell, and ran some commands like `cmd1 | cmd2 | cmd3 | cmd4`. Then this
 is a process group in this session.

It's easy to run multiple groups like above. All of these process groups belongs to same
 session.

The one that interacts with the terminal tty is the foreground one and all others are
 background process groups. The control terminal (tty) can be transferred from one to
 another.

### create new session and group
You can use `pid_t setsid(void);` to create a new session. The current calling process
 becomes the new session leader. This new session has a process group and the calling
 process is the group leader too.

You'll fail to create new session if the calling process itself is already session leader.

### change group
You can change or create process group using the `int setpgid(pid_t pid, pid_t pgid);`
 function. But there are some rules:

- process can only change process group of itself or its children, it cannot change others
 even though it is root

- session leader cannot change process group of itself

- cannot move process to group of other session

### orphaned process groups
When a session leader terminated, the left process groups may become orphaned process groups.
Actually, a process group is orphaned when there isn't any process that its parent is in other
 group of same session.

The source code to check this is at `kernel/exit.c`:

```c
static int will_become_orphaned_pgrp(struct pid *pgrp,
					struct task_struct *ignored_task)
{
	struct task_struct *p;

	do_each_pid_task(pgrp, PIDTYPE_PGID, p) {
		if ((p == ignored_task) ||
		    (p->exit_state && thread_group_empty(p)) ||
		    is_global_init(p->real_parent))
			continue;

		if (task_pgrp(p->real_parent) != pgrp &&
		    task_session(p->real_parent) == task_session(p))
			return 0;
	} while_each_pid_task(pgrp, PIDTYPE_PGID, p);

	return 1;
}
```

The session leader is usually a shell. After it is existed, other processes cannot receive
 user instructions. And some background process groups may in stopped state. They will keep
 stopped and use system resources if there is no special handling.

To avoid this situation, kernel will send a `SIGHUP` signal and then a `SIGCONT` signal to
 each process if there is stopped process in this group.

You may wonder why `SIGCONT`. We know that the default action of SIGHUP is termination. But
 process can capture this signal can choose to keep running. But these processes may be
 suspended previously (by CTRL-Z). Then the `SIGCONT` signal can awake them and keep them
 running.


### terminal and signals
When the connection to control terminal is lost, kernel will send `SIGHUP` signal to leader
 of the foreground process group of the session. Usually, the leader is the shell. Then
 the shell decides how to deal with processes of this session.

Usually, the shell notifies others using the `SIGHUP` signal. The default action of `SIGHUP`
 is termination.

### references
- [linux process](https://www.win.tue.nl/~aeb/linux/lk/lk-10.html)
