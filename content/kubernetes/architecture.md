```metadata
tags: kubernetes, architecture
```

## kubernetes architecture


### why kubernetes
    docker      ----->         kubernetes

### components
![architecture](./images/kubernetes-architecture.png)

    - etcd
        + data store
        + HA, raft, multi replica
    - apiserver
        + expose APIs (CURD)
        + easy to scale up
    - scheduler
        + how to schedule a pod
        + which node to choose
        + affinity/anti-affinity
        + priority
    - controller manager
        + deployment
        + replicaset
        + statefulset
        + daemonset
        + job
        + and so on
    - kubelet
        + node agent run on each node
        + run pods and watch pod states (CRI, CNI)
        + report node states
    - kubeproxy
        + clusterIP to podIP (service to pod)
        + iptables or IPVS
    - CRI
        + container runtime (docker, containerd, rkt and ...)

### HA design
    - etcd cluster to make data store safe
    - apiserver is stateless and can easily scale up
    - you can run multiple scheduler and controller managers
        + only leader will do the actual work
        + lead election
    - rate limit everywhere
    - special tricks to large scale cluster

### scheduler
    - event driving (eventhandlers.go)
    - scheduling queue (active, backoff, unschedulable)
    - loop to deal with the queue

### assign node to pod
![scheduling framework extensions](./images/scheduling-framework-extensions.png)

    - pluggable
    - multiple stage
    - easy to extend your own logics
    - scheduling algorithm
        + find a list of suggested nodes
        + extendable with http api
        + compute score (some plugins may have Score function to give a score)
        + choose the best one
    - deal with large scale cluster
        + only test with limited nodes each time
        + round robin all nodes

### summary
    - learn whole picture of kubernetes
    - how to design a large scale system
    - learn good coding styles and patterns
        + context
        + wait.XXX
        + rateLimit, queue
        + pluggable design
        + testable code (interface, fake implementation, time.Clock)

### references
- [k8s official: kubernetes components](https://kubernetes.io/docs/concepts/overview/components/)
