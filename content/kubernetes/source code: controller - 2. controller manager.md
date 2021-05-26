```metadata
tags: kubernetes, sourcecode, controller
```

## kubernetes sourcecode: controller manager

`controller manager` is a component of the control plane. It runs as a standalone process
 that iniatializing and starting all controllers.


The `NewControllerManagerCommand` defines a `cobra` command which simply wraps the `Run()`
 function.

```go
// kubernetes: cmd/kube-controller-manager/app/controllermanager.go

// NewControllerManagerCommand creates a *cobra.Command object with default parameters
func NewControllerManagerCommand() *cobra.Command {
	s, err := options.NewKubeControllerManagerOptions()

	cmd := &cobra.Command{
		Run: func(cmd *cobra.Command, args []string) {
			c, err := s.Config(KnownControllers(), ControllersDisabledByDefault.List())

			if err := Run(c.Complete(), wait.NeverStop); err != nil {
				fmt.Fprintf(os.Stderr, "%v\n", err)
				os.Exit(1)
			}
		},
	}

	return cmd
}
```

Like scheduler, you can have multiple controller managers in a cluster too. But only one
 will do the actual job. They will choose a lead controller managers via election.

The core part of the `Run()` is the function that starts all the controllers via
 `StartControllers`. It also starts related informers and deals with election.

The the main goroutine needs to run forever so there is a `select {}` at end of the `run` function.
```go
// kubernetes: cmd/kube-controller-manager/app/controllermanager.go

func Run(c *config.CompletedConfig, stopCh <-chan struct{}) error {

	run := func(ctx context.Context, startSATokenController InitFunc, initializersFunc ControllerInitializersFunc) {
		controllerContext, err := CreateControllerContext(c, rootClientBuilder, clientBuilder, ctx.Done())
		if err != nil {
			klog.Fatalf("error building controller context: %v", err)
		}
		controllerInitializers := initializersFunc(controllerContext.LoopMode)
		if err := StartControllers(controllerContext, startSATokenController, controllerInitializers, unsecuredMux); err != nil {
			klog.Fatalf("error starting controllers: %v", err)
		}

		controllerContext.InformerFactory.Start(controllerContext.Stop)
		controllerContext.ObjectOrMetadataInformerFactory.Start(controllerContext.Stop)
		close(controllerContext.InformersStarted)

		select {}
	}

	// No leader election, run directly
	if !c.ComponentConfig.Generic.LeaderElection.LeaderElect {
		run(context.TODO(), saTokenControllerInitFunc, NewControllerInitializers)
		panic("unreachable")
	}
}
```

The `StartControllers` starts the `SA token controller` controller at first since it needs
 to setup permissions for other controllers. And then starts all other controllers one by
 one.

Each controller has `startXXXController` function that will start it. The function
 `NewControllerInitializers` returns a map of all except the `SA token controller`.

```go
func NewControllerInitializers(loopMode ControllerLoopMode) map[string]InitFunc {
	controllers := map[string]InitFunc{}
	controllers["endpoint"] = startEndpointController
	controllers["endpointslice"] = startEndpointSliceController
	controllers["endpointslicemirroring"] = startEndpointSliceMirroringController
	controllers["replicationcontroller"] = startReplicationController
	controllers["podgc"] = startPodGCController
	controllers["resourcequota"] = startResourceQuotaController
	controllers["namespace"] = startNamespaceController
	controllers["serviceaccount"] = startServiceAccountController
	controllers["garbagecollector"] = startGarbageCollectorController
	controllers["daemonset"] = startDaemonSetController
	controllers["job"] = startJobController
	controllers["deployment"] = startDeploymentController
	controllers["replicaset"] = startReplicaSetController
	controllers["horizontalpodautoscaling"] = startHPAController
	controllers["disruption"] = startDisruptionController
	controllers["statefulset"] = startStatefulSetController
	controllers["cronjob"] = startCronJobController
	controllers["csrsigning"] = startCSRSigningController
	controllers["csrapproving"] = startCSRApprovingController
	controllers["csrcleaner"] = startCSRCleanerController
	controllers["ttl"] = startTTLController
	controllers["bootstrapsigner"] = startBootstrapSignerController
	controllers["tokencleaner"] = startTokenCleanerController
	controllers["nodeipam"] = startNodeIpamController
	controllers["nodelifecycle"] = startNodeLifecycleController
	if loopMode == IncludeCloudLoops {
		controllers["service"] = startServiceController
		controllers["route"] = startRouteController
		controllers["cloud-node-lifecycle"] = startCloudNodeLifecycleController
		// TODO: volume controller into the IncludeCloudLoops only set.
	}
	controllers["persistentvolume-binder"] = startPersistentVolumeBinderController
	controllers["attachdetach"] = startAttachDetachController
	controllers["persistentvolume-expander"] = startVolumeExpandController
	controllers["clusterrole-aggregation"] = startClusterRoleAggregrationController
	controllers["pvc-protection"] = startPVCProtectionController
	controllers["pv-protection"] = startPVProtectionController
	controllers["ttl-after-finished"] = startTTLAfterFinishedController
	controllers["root-ca-cert-publisher"] = startRootCACertPublisher
	controllers["ephemeral-volume"] = startEphemeralVolumeController
	if utilfeature.DefaultFeatureGate.Enabled(genericfeatures.APIServerIdentity) &&
		utilfeature.DefaultFeatureGate.Enabled(genericfeatures.StorageVersionAPI) {
		controllers["storage-version-gc"] = startStorageVersionGCController
	}

	return controllers
}
```

Some controllers are disabled by default.

```go
var ControllersDisabledByDefault = sets.NewString(
	"bootstrapsigner",
	"tokencleaner",
)
```

### start controller
Let's take the `TTL controller` as an example to see how the `startXXXController` works.
It simply `new` a controller with informer and kube client and runs it with multiple workers
 in a goroutine.

```go
func startTTLController(ctx ControllerContext) (http.Handler, bool, error) {
	go ttlcontroller.NewTTLController(
		ctx.InformerFactory.Core().V1().Nodes(),
		ctx.ClientBuilder.ClientOrDie("ttl-controller"),
	).Run(5, ctx.Stop)
	return nil, true, nil
}
```

### references
- [k8s official: controller](https://kubernetes.io/docs/concepts/architecture/controller/)
- [k8s source code: controllermanager](https://github.com/kubernetes/kubernetes/blob/v1.19.0/cmd/kube-controller-manager/app//controllermanager.go)
