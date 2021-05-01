```metadata
tags: kubernetes, scheduling, priority, preemption
```

## kubernetes scheduling: priority and preemption
Resources are limited. Sometimes, you may find there isn't enough resources to deploy
 a new important pod. You can destroy some less important pods to make room for the
 important one. But can kubernetes handle this automatically?

The answer is yes. You can set priority on pod. Then if there isn't enough resources,
 kubernetes will try to evict lower priority pods so that higher priority pod can be
 scheduled.

You can define an PriorityClass like following (official example):

```yaml
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: high-priority
value: 1000000
globalDefault: false
description: "This priority class should be used for XYZ service pods only."
```

Then you can refer to this PriorityClass in pod using `priorityClassName` like following:

```yaml
kind: Pod
spec:
  containers:
  - name: nginx
    image: nginx
    imagePullPolicy: IfNotPresent
  priorityClassName: high-priority
```

Then the pod has a priority of 1000000. The largest priority value that user can use
 if 1 billion. Value that is large than this is reserved by kubernetes for internal
 pods.

### globalDefault
You can define one PriorityClass with `globalDefault: true` then pod that hasn't
 specified a priorityClassName will use this default one.

### PreemptionPolicy
PriorityClass has a field `PreemptionPolicy` that you can use it to control whether it
 prempts or not. The default value is `PreemptLowerPriority` which means that it will
 try to prempt lower priority pod.

You can set it to `Never` so that it will be placed ahead of lower priority pods in the
 scheduling queue. And it **won't prempt** lower priority pods.

### built-in PriorityClass
There are two built-in PriorityClasses currently: `system-cluster-critical` and
 `system-node-critical`. The former one has a priority of 2 billion while the latter
 one has a higher priority of 2 billion and 1 thousand. Both of them can prempt lower
 priority pods.

### attentions
Preemption **won't** happen if higher priority pod still cannot be scheduled after
 the preemption. For example, the higher pod may has affinity of the lower one so that
 the affinity will not be satisfied if the lower priority pod is preempted.

### references
- [k8s official: pod priority and preemption](https://kubernetes.io/docs/concepts/configuration/pod-priority-preemption/)
