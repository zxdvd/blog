```metadata
tags: kubernetes, ops
```

## kubernetes operations: too many evicted pods

You may find that kubernetes keeps the records of evicted pods. And it may accumulate too
 much evicted pods. It's really annoying.

It's easy to find a lot of shell commands to do the clean up. But you may wonder that
 why not clean old evicted pods automatically?

Actually, there is a controller that deals with this situation. The `podgc` controller
 will clean up thosed terminated pods, orphaned pods and unscheduling terminated pods.

However, for terminated pods, kubernetes has a threshold that it won't garbage collect
 if count of terminated pods is less than the threshold. The problem is the default
 threshold, 12500, is too large, especially for small cluster.

You can change it via the `--terminated-pod-gc-threshold` option of `controller-manager`.
 You can set it to 100, 500 or any other value.

### references
- [k8s issue: kubelet does not delete evicted pods](https://github.com/kubernetes/kubernetes/issues/55051)
