```metadata
tags: linux, container, cgroup
```

## cgroup freezer

I kernel document about this is very clear.

We can use cgroup freezer to freeze a group of processes.

It has 3 state: THAWED, FREEZING, FROZEN. You can read state from `freezer.state`.
 And you can write state to that file to change state. But only `THAWED` and `FROZEN`
 can be written. `FREEZING` is a internal state that processes will go to the `FREEZING`
 at first and then `FROZEN`.

If a cgroup state is frozen, then all processes of it and its descendant cgroups are
 frozen. So a descendant cgroup may be frozen even if state of itself is `THAWED`.

Then kernel provides `freezer.self_freezing` and `freezer.parent_freezing` to indicate
 that whether a cgroup itself is frozen or its parent is frozen.


#### cgroup2 freezer
For cgroup v1, the freezer may put the process into unkillable state when the process
 is in uninterruptible state. Then you cannot kill it. The cgroup v2 changes this to
 make it more like the SIGSTOP, it will freeze when the process is in killable state.

### references
- [kernel doc: freezer subsystem](https://www.kernel.org/doc/Documentation/cgroup-v1/freezer-subsystem.txt)
