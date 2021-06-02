```metadata
tags: kubernetes, sourcecode, controller
```

## kubernetes sourcecode: podgc controller

For a long running cluster, it will produce a lot of garbage pods. And the `podgc` controller
 is responsive to handling the garbage collection.

The controller runs `gcc.gc` every 20 seconds. And the `gcc.gc` deals with 3 kinds of
 garbages: terminated pods, orphaned pods, unscheduled terminating pods. Orphaned pod
 is pod that related node doesn't exist. Unscheduled terminating pod is pod that hasn't
 been scheduled and is terminating.

```go
func (gcc *PodGCController) Run(stop <-chan struct{}) {
	go wait.Until(gcc.gc, gcCheckPeriod, stop)

	<-stop
}

func (gcc *PodGCController) gc() {
	pods, err := gcc.podLister.List(labels.Everything())
	nodes, err := gcc.nodeLister.List(labels.Everything())
	if gcc.terminatedPodThreshold > 0 {
		gcc.gcTerminated(pods)
	}
	gcc.gcOrphaned(pods, nodes)
	gcc.gcUnscheduledTerminating(pods)
}
```

For terminated pods, kubernetes has a default threshold that it won't delete them if
 total number of terminated pods is less than the threshold. And it will sort them
 by `creationTimestamp` (I don't know why not use deletionTimestamp) so that terminated
 pod that created earlier will be deleted earlier.

Attention, the default threshold 12500 is very large which means that it won't delete
 anyone until you have more than 12500 terminated pods. For small cluster you'd better
 use a small threshold. The `--terminated-pod-gc-threshold` option of controller
 manager can be used to set it.

### references
- [k8s source code: podgc controller](https://github.com/kubernetes/kubernetes/blob/v1.19.0/pkg/controller/podgc/gc_controller.go)
