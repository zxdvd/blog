```metadata
tags: kubernetes, sourcecode, scheduling
```

## kubernetes source code: scheduling overview

The scheduler is run at control plane to handle pod scheduling. It is a standalone
 process. Following is the scheduler on my single node cluster:

    kube-scheduler --authentication-kubeconfig=/etc/kubernetes/scheduler.conf
            --authorization-kubeconfig=/etc/kubernetes/scheduler.conf
            --bind-address=127.0.0.1
            --kubeconfig=/etc/kubernetes/scheduler.conf
            --leader-elect=true --port=0

The command line package deals with arguments and configurations. It sets up a `Scheduler`
 and calls `sched.Run()`.

The `sched.Run()` runs a dead loop and calls `sched.scheduleOne` again and again.

```go
// Run begins watching and scheduling. It starts scheduling and blocked until the context is done.
func (sched *Scheduler) Run(ctx context.Context) {
	sched.SchedulingQueue.Run()
	wait.UntilWithContext(ctx, sched.scheduleOne, 0)
	sched.SchedulingQueue.Close()
}
```

The `scheduleOne()` is the core executing part of scheduler. It tries to pick a pod
 from the scheduling queue. Then the scheduling algorithm will give a suggested node
 for the pod.

And then a lot of filters (like node/pod affinity) are applied.

If checking of all filters passed, it goes to the node binding process. The binding
 also contains multiple stages: WaitOnPermit, PreBind, Bind, PostBind.

![scheduling framework extensions](./images/scheduling-framework-extensions.png)

```go
// scheduleOne does the entire scheduling workflow for a single pod. It is serialized on the scheduling algorithm's host fitting.
func (sched *Scheduler) scheduleOne(ctx context.Context) {
	podInfo := sched.NextPod()
	// pod could be nil when schedulerQueue is closed
	if podInfo == nil || podInfo.Pod == nil {
		return
	}
	pod := podInfo.Pod
	fwk, err := sched.frameworkForPod(pod)
	if err != nil {
		klog.ErrorS(err, "Error occurred")
		return
	}
	if sched.skipPodSchedule(fwk, pod) {
		return
	}

	// Synchronously attempt to find a fit for the pod.
	state := framework.NewCycleState()
	schedulingCycleCtx, cancel := context.WithCancel(ctx)
	defer cancel()
	scheduleResult, err := sched.Algorithm.Schedule(schedulingCycleCtx, fwk, state, pod)
	if err != nil {
        // ## handle error
		return
	}
	// Tell the cache to assume that a pod now is running on a given node, even though it hasn't been bound yet.
	// This allows us to keep scheduling without waiting on binding to occur.
	assumedPodInfo := podInfo.DeepCopy()
	assumedPod := assumedPodInfo.Pod
	// assume modifies `assumedPod` by setting NodeName=scheduleResult.SuggestedHost
	err = sched.assume(assumedPod, scheduleResult.SuggestedHost)
	if err != nil {
        // ## handle error
		return
	}

	// Run the Reserve method of reserve plugins.
	if sts := fwk.RunReservePluginsReserve(schedulingCycleCtx, state, assumedPod, scheduleResult.SuggestedHost); !sts.IsSuccess() {
        // ## handle error
		return
	}

	// Run "permit" plugins.
	runPermitStatus := fwk.RunPermitPlugins(schedulingCycleCtx, state, assumedPod, scheduleResult.SuggestedHost)
	if runPermitStatus.Code() != framework.Wait && !runPermitStatus.IsSuccess() {
        // ## handle error
		return
	}

	// bind the pod to its host asynchronously (we can do this b/c of the assumption step above).
	go func() {
		bindingCycleCtx, cancel := context.WithCancel(ctx)
		defer cancel()

		waitOnPermitStatus := fwk.WaitOnPermit(bindingCycleCtx, assumedPod)
		if !waitOnPermitStatus.IsSuccess() {
            // ## handle error
			return
		}

		// Run "prebind" plugins.
		preBindStatus := fwk.RunPreBindPlugins(bindingCycleCtx, state, assumedPod, scheduleResult.SuggestedHost)
		if !preBindStatus.IsSuccess() {
            // ## handle error
			return
		}

		err := sched.bind(bindingCycleCtx, fwk, assumedPod, scheduleResult.SuggestedHost, state)
		if err != nil {
            // ## handle error
		} else {
			// Run "postbind" plugins.
			fwk.RunPostBindPlugins(bindingCycleCtx, state, assumedPod, scheduleResult.SuggestedHost)
		}
	}()
}
```

### references
- [k8s official: Scheduling Framework](https://kubernetes.io/docs/concepts/scheduling-eviction/scheduling-framework/)
