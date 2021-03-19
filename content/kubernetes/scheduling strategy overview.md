```metadata
tags: kubernetes, scheduling, affinity, taint
```

## kubernetes: scheduling strategy overview

Kubernetes abstracts all your resources and schedules tasks to nodes according to required
 resources. You don't need to know which node the task is running.

However, nodes may vary from one to another, some may have GPUs, or strong CPUs some may
 not. And same to pods too, some pods may need GPUs or strong CPUs.

Kubernetes provides some strategies that you can use to influence the scheduler to achieve
 specific scheduling requirements.

### nodeName
`nodeName` is the simplest strategy. You can set `nodeName` on pod spec so that scheduler
 will only schedule this pod to specific node.

A example:

```yaml
spec:
    nodeName: master
    containers:
        ...
```

### nodeSelector
You can use `nodeSelector` to select a group of nodes. Like

```yaml
spec:
    nodeSelector:
        regin: 'beijing'
        cpu: 'intel'
    containers:
        ...
```

### affinity and anti-affinity
The `affinity` and `anti-affinity` are more advanced strategies. They provide powerful
 features like:

    - they provide multiple affinity types to indicate soft or hard policy, like
        * requiredDuringSchedulingIgnoredDuringExecution
        * preferredDuringSchedulingIgnoredDuringExecution
        * requiredDuringSchedulingRequiredDuringExecution (planned)
    - anti-affinity can used to exclude nodes
    - support logical operations AND, OR
    - support multiple matching operators like `In`, `NotIn`, `Exists`, `DoesNotExist`, `Gt`, `Lt`

The `affinity` and `anti-affinity` can be used to nodes and pods. So there are
 `nodeAffinity`, `nodeAntiAffinity`, `podAffinity` and `podAntiAffinity`.

### nodeAffinity and nodeAntiAffinity
`nodeAffinity` and `nodeAntiAffinity` are used to define that whether the pods can be scheduled
 to nodes based on labels on each node.

A example from official document:

```yaml
spec:
    affinity:
        nodeAffinity:
            requiredDuringSchedulingIgnoredDuringExecution:
                nodeSelectorTerms:
                - matchExpressions:
                    - key: kubernetes.io/e2e-az-name
                      operator: In
                      values:
                      - e2e-az1
                      - e2e-az2
    containers:
        ...
```

### podAffinity and podAntiAffinity
`podAffinity` and `podAntiAffinity` are similar to `nodeAffinity` and `nodeAntiAffinity`.
 The difference is that `podAffinity` matches labels against labels of other pods in each
 node instead of labels of node itself.

You can use `podAffinity` to deploy services that have strong relations to same node,
 like web server and cache server, web server and database proxy. By this way, RPCs
 between these services can be faster.

The `podAntiAffinity` can be used to deploy a service evenly. For example, you can deploy
 cache server that anti-affinity with itself so that you won't have two cache servers
 deployed at same node.

#### topologyKey
For pod affinity/anti-affinity, you need to specify a `topologyKey`, it is node label.
 The affinity/anti-affinity applies in nodes that have same values of the topologyKey.

### taints and tolerations
`affinity` is used for pods to choose nodes while `taints` is used for nodes to choose
 pods.

You can taint a node and then it will reject all pods scheduling by default. Only pods
 with matched tolerations are allowed to schedule.

For example, master nodes are tainted with `node-role.kubernetes.io/master:NoSchedule`
 so that pods won't be scheduled on them by default. If you want to run some pods on
 it, you can add tolerations like following:

```yaml
tolerations:
- key: "node-role.kubernetes.io/master"
  operator: "Exists"
  effect: "NoSchedule"
```

A node may have many taints and you need to have tolerations that match all the taints
 to schedule pods on it.

### node selector vs taint
It seems that both node selector and taint can bind pods and nodes together. So what's
 the difference?

For node selector or node affinity, it's the pod choose which nodes it could be scheduled
 to. It can only go to limited nodes. It doesn't prevent pods without labels going to
 those nodes.

For taint, the nodes taint themselves with specific labels and only pods with proper
 tolerations can be scheduled to these nodes. It doesn't prevent tainted pods going to
 other untained nodes.

You can use labels when pods have requirements like SSD, GPU so that it can be scheduled
 to proper nodes. You can use taint to reserve resources, for example, master nodes are
 tainted so that normal pods are not able to be scheduled to masters.

Sometimes, you need to combine them together. For example, you should use GPU labels
 so that pods need GPU can go to nodes have GPUs. And you also need to taint related
 nodes with GPU so that normal pods won't go to GPU nodes since GPU is expensive
 resource.

### references
- [k8s official: nodeSelector and affinity](https://kubernetes.io/docs/concepts/scheduling-eviction/assign-pod-node)
- [k8s official: taint and toleration](https://kubernetes.io/docs/concepts/scheduling-eviction/taint-and-toleration/)
