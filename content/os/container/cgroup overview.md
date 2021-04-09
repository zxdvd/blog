```metadata
tags: linux, container, cgroup
```

## cgroup overview

### termilonogy
- controller: cgroup has many controllers (memory, cpu, cpuset, cpuacct, io), each
 controls a kind of resource.

### cgroup1 and cgroup2
Due to problems of `cgroup v1` (current used), `cgroup v2` started at linux 3.10 and
 released at linux 4.5. Now a lot of distribution enable it and systemd mounts it.

However, it is far more to say that cgroup v2 is replacing cgroup v1. The key point
 is that they can co-exist but a controller can only exist in one side. If you enabled
 `memory` controller in cgroup v2 then you cannot use it in cgroup v1. If any applications
 use cgroup v1 and enable some controllers, then it blocks you to use cgroup v2. Systemd
 do enabling cgroup v2 but it doesn't enable any controller.

I think it may still need years to upgrade to cgroup v2. So it is still necessary to
 learn about cgroup v1. And they share many common features too.

#### cgroup v1 problems and cgroup v2 improvements
In cgroup v1, each controller is a standalone hierarchy tree. You may have near 12
 controller trees. Like following image:

![cgroup v1 hierarchy](./images/cgroup-v1.jpg)

From above we know that you need to create sub-directories in each controller hierarchy and
 then move processes to each directory when you want to start a container with resource
 limitation.

Another problem is that you can add process to any node of the hierarchy tree in cgroup v1.
Then it is confused that how the resources were divided between processes in a node and
children of the node.

The cgroup v2 which was released in kernel 4.5 (2016.03) takes things easy by using a
 single hierarchy tree like following:

![cgroup v2 hierarchy](./images/cgroup-v2.png)

Each single node of the tree is a conbination of multiple controllers. And you can only
 add processes to leaf node of the tree. By this way, it avoids the situation of dividing
 resources between node itself and children.

It's more suitable for containers. Just create one node for a pod and all its processes
 is limited using this node.


### references
- [very detailed man page about cgroup1 and cgroup2](https://man7.org/linux/man-pages/man7/cgroups.7.html)
- [kernel doc: cgroup v1 memory](https://www.kernel.org/doc/Documentation/cgroup-v1/memory.txt)
- [kernel doc: cgroup v2](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html)

