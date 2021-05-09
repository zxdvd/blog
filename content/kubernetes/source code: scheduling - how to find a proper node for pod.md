```metadata
tags: kubernetes, sourcecode, scheduling
```

## kubernetes source code: scheduling - how to find a proper node for pod

From article [scheduling overview](./source code: scheduling overview.md), we know that
 the `scheduleOne()` is used to schedule a pod in the queue.

It calls `sched.Algorithm.Schedule()` to find the suggested node for the pod.

    scheduleResult, err := sched.Algorithm.Schedule(schedulingCycleCtx, fwk, state, pod)

This article will focus on how the scheduling algorithm works to find the node.

The scheduling algorithm is an interface that defines as following:

```go
type ScheduleAlgorithm interface {
	Schedule(context.Context, framework.Framework, *framework.CycleState, *v1.Pod) (scheduleResult ScheduleResult, err error)
	// Extenders returns a slice of extender config. This is exposed for
	// testing.
	Extenders() []framework.Extender
}
```

Kubernetes has a generic scheduler that implements above interface. The main steps of the scheduling are:

- if node number is 0, returns `ErrNoNodesAvailable`
- find feasible nodes
    - if no feasible node, returns error
    - if only one feasible node, returns it as suggested host
    - if there are more than one, computes score of each node and chooses the best one

```go
// kubernetes: pkg/scheduler/core/generic_scheduler.go
func (g *genericScheduler) Schedule(ctx context.Context, fwk framework.Framework, state *framework.CycleState, pod *v1.Pod) (result ScheduleResult, err error) {
	if g.nodeInfoSnapshot.NumNodes() == 0 {
		return result, ErrNoNodesAvailable
	}

	feasibleNodes, diagnosis, err := g.findNodesThatFitPod(ctx, fwk, state, pod)

	if len(feasibleNodes) == 0 {
		return result, &framework.FitError{}
	}

	// When only one node after predicate, just use it.
	if len(feasibleNodes) == 1 {
		return ScheduleResult{
			SuggestedHost:  feasibleNodes[0].Name,
			EvaluatedNodes: 1 + len(diagnosis.NodeToStatusMap),
			FeasibleNodes:  1,
		}, nil
	}

    // // score feasible nodes
	priorityList, err := g.prioritizeNodes(ctx, fwk, state, pod, feasibleNodes)

    // // choose the node with highest score
	host, err := g.selectHost(priorityList)

	return ScheduleResult{
		SuggestedHost:  host,
		EvaluatedNodes: len(feasibleNodes) + len(diagnosis.NodeToStatusMap),
		FeasibleNodes:  len(feasibleNodes),
	}, err
}
```

The `findNodesThatFitPod` is called to find feasible nodes. It applies `prefilter` at
 first. And then tries with the nominated node if the pod has one.

And then it applies `filters` and `extenders` onto all nodes (it may use only part of nodes).

```go
func (g *genericScheduler) findNodesThatFitPod(ctx context.Context, fwk framework.Framework, state *framework.CycleState, pod *v1.Pod) ([]*v1.Node, framework.Diagnosis, error) {
	// Run "prefilter" plugins.
	s := fwk.RunPreFilterPlugins(ctx, state, pod)

	// "NominatedNodeName" can potentially be set in a previous scheduling cycle as a result of preemption.
	// This node is likely the only candidate that will fit the pod, and hence we try it first before iterating over all nodes.
	if len(pod.Status.NominatedNodeName) > 0 && feature.DefaultFeatureGate.Enabled(features.PreferNominatedNode) {
		feasibleNodes, err := g.evaluateNominatedNode(ctx, pod, fwk, state, diagnosis)
		// Nominated node passes all the filters, scheduler is good to assign this node to the pod.
		if len(feasibleNodes) != 0 {
			return feasibleNodes, diagnosis, nil
		}
	}
	feasibleNodes, err := g.findNodesThatPassFilters(ctx, fwk, state, pod, diagnosis, allNodes)

	feasibleNodes, err = g.findNodesThatPassExtenders(pod, feasibleNodes, diagnosis.NodeToStatusMap)

	return feasibleNodes, diagnosis, nil
}
```

### huge cluster
It seems that we need to apply all filters and extenders to every node in the cluster.
 It may not be a problem if it is a small cluster. However, what about a huge cluster
 with thousands of nodes? It may take too much time to schedule a single pod.

To avoid this problem, kubernetes will only loop some nodes of large cluster. You can
 set a percentage like 30% so that it will use 30% of total nodes.

If you didn't set it, it will calcuate the percentage as following:

    basePercentageOfNodesToScore := int32(50)
    adaptivePercentage = basePercentageOfNodesToScore - numAllNodes/125

So for a cluster with 1000 nodes, the percentage will be 42%. It has a minimum percentage: 5%
 and minimum nodes: 100. So if it is a small cluster (nodes less than 100), all nodes
 will be used. Or if the calcuated result is less than 100, at least 100 nodes will be
 used.

You may be concerned about that some nodes may be picked always while other nodes are never
 picked for huge cluster.

Actually, the scheduler uses the `nextStartNodeIndex` field to remember the start node of
 next picking. So that all nodes will be picked in round turn.

### references
- [k8s official: Scheduling Framework](https://kubernetes.io/docs/concepts/scheduling-eviction/scheduling-framework/)
