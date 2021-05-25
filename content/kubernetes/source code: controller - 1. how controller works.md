```metadata
tags: kubernetes, sourcecode, controller
```

## kubernetes sourcecode: controller - how controller works

The official document defines controllers as following:

    In Kubernetes, controllers are control loops that watch the state of your cluster,
    then make or request changes where needed. Each controller tries to move the current
    cluster state closer to the desired state.

The control plane has a component `controller-manager`. It will initiate and start all
 controllers. For each controller, the manager may start one or more workers. Following
 are common controllers:

- deployment: deploy a set of pods
- replicaset: deal with replicaset changes (start more pods or terminate some)
- statefulset: deal with stateful pods
- daemonset
- job
- cronjob

### TTL controller as an example

You can find source code of each controller at `kubernetes/pkg/controller`. I found a
 very simple controller, `ttl/ttl_controller.go`. It uses less than 300 lines of code,
 including comments, spaces and imports.

The aim of this controller is to adjust the TTL value of each node according to the
 cluster size (total nodes of the cluster). The TTL value is stored as an annotation
 of each node object. It is used to specify that how long the `kubelet` needs to refresh
 secrets and configmaps that linked to the node.

The TTL value varies according to cluster size. For large scale cluster, it uses bigger
 TTL value so that `kubelet` won't refresh so frequently so that it will reduce pressure
 of `apiserver` a lot.

For cluster that has more than 9000 nodes, the TTL is 600 seconds. For cluster that has
 nodes between 90 and 500, it is 15 seconds.

```go
var (
	ttlBoundaries = []ttlBoundary{
		{sizeMin: 0, sizeMax: 100, ttlSeconds: 0},
		{sizeMin: 90, sizeMax: 500, ttlSeconds: 15},
		{sizeMin: 450, sizeMax: 1000, ttlSeconds: 30},
		{sizeMin: 900, sizeMax: 2000, ttlSeconds: 60},
		{sizeMin: 1800, sizeMax: 10000, ttlSeconds: 300},
		{sizeMin: 9000, sizeMax: math.MaxInt32, ttlSeconds: 600},
	}
)
```

Following is the defined data struct for the TTL controller. The controller will get
 a whole list of nodes and then deal with node changes like add or delete. The `hasSynced`
 is a function to notify that whether it has got the whole node list or not.

By default, the controller manager will start 5 worker for the TTL controller. So it
 needs the `lock` to protect fields like `nodeCount`.

```go
type Controller struct {
	kubeClient clientset.Interface
	// nodeStore is a local cache of nodes.
	nodeStore listers.NodeLister
	// Nodes that need to be synced.
	queue workqueue.RateLimitingInterface
	// Returns true if all underlying informers are synced.
	hasSynced func() bool
	lock sync.RWMutex
	// Number of nodes in the cluster.
	nodeCount int
	// Desired TTL for all nodes in the cluster.
	desiredTTLSeconds int
	// In which interval of cluster size we currently are.
	boundaryStep int
}
```

For this controller, it needs to adjust TTL value when cluster size changed. So it needs
 to watch changes of node, like `addNode`, `updateNode`, `deleteNode`. Then computes the
 TTL value accordingly.

```go
// NewTTLController creates a new TTLController
func NewTTLController(nodeInformer informers.NodeInformer, kubeClient clientset.Interface) *Controller {
	ttlc := &Controller{
		kubeClient: kubeClient,
		queue:      workqueue.NewNamedRateLimitingQueue(workqueue.DefaultControllerRateLimiter(), "ttlcontroller"),
	}

	nodeInformer.Informer().AddEventHandler(cache.ResourceEventHandlerFuncs{
		AddFunc:    ttlc.addNode,
		UpdateFunc: ttlc.updateNode,
		DeleteFunc: ttlc.deleteNode,
	})

	ttlc.nodeStore = listers.NewNodeLister(nodeInformer.Informer().GetIndexer())
	ttlc.hasSynced = nodeInformer.Informer().HasSynced

	return ttlc
}
```

The `NewTTLController` only initiates a TTL controller. The `Run()` method starts
 multiple workers concurrently and deals with graceful shutdown.

```go
func (ttlc *Controller) Run(workers int, stopCh <-chan struct{}) {
	defer utilruntime.HandleCrash()
	defer ttlc.queue.ShutDown()

	klog.Infof("Starting TTL controller")
	defer klog.Infof("Shutting down TTL controller")

	if !cache.WaitForNamedCacheSync("TTL", stopCh, ttlc.hasSynced) {
		return
	}

	for i := 0; i < workers; i++ {
		go wait.Until(ttlc.worker, time.Second, stopCh)
	}

	<-stopCh
}
```

For the actual `addNode`, `updateNode`, `deleteNode` callbacks. They just compute and
 update the TTL seconds of this controller. And then they call the `ttlc.enqueueNode(node)`
 to enqueue the changed node.

The worker will get node from the queue and update the annotation of the node object.

```go
func (ttlc *Controller) worker() {
	for ttlc.processItem() {
	}
}
```

The `processItem` will get one node from queue and update TTL annotation accordingly.

### summary
A controller will watch one or more kinds of objects and run a control loop to deal
 with the changes in order to keep the cluster state in sync with the data store (etcd).

### references
- [k8s official: controller](https://kubernetes.io/docs/concepts/architecture/controller/)
- [k8s source code: TTL controller](https://github.com/kubernetes/kubernetes/blob/v1.19.0/pkg/controller/ttl/ttl_controller.go)
