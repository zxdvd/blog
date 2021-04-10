```metadata
tags: kubernetes, resource, limit, quota, memory, cpu, qos, oom
```

## kubernetes resource manage: QOS

Kubernetes supports QOS (Quality of Service) by dividing pods into 3 different levels:
 `Guaranteed`, `Burstable` and `BestEffort`.

Kubernetes doesn't support set QOS level of a pod directly since you cannot guarantee
 a pod will run without problem if you don't know how much resources it needs.

Kubernetes automatically compute the QOS level via requests and limits settings of pods.
Currently, it only counts CPU and memory.

If **each** container in a pod has request and limit for both cpu and memory, and the
 request and memory are **same**. Then this pod will be classed as `Guaranteed`.

If there is neither request nor limit for both cpu and memory and **any** container of
 a pod, then it is `BestEffort`.

For others in the middle, it means they have at least one request or limit for cpu or
 memory of a container, then it is `Burstable`.

`Guaranteed` is the highest level. Kubernetes guarantees resources for those pods. Then
 is the `Burstable` level. And the `BestEffort` is the lowest level. Kubernetes will
 try best for them if there have enough resources.

### how Kubernetes implements QOS
Kubernetes uses cgroups to manage cpu and memory resources. For cpu, cgroups can limit
 cpu share time of processes. A process will hang until next time slice if it uses out
 its cpu share time.

But for memory, it will trigger OOM if it reaches limit. OOM killer will kill the process
 which has the largest oom_score (1000). Kubernetes achieves the QOS by giving different
 oom_score for processes in different QOS level.

Currently, Kubernetes defines oom score adjust as following:

```go
	// KubeletOOMScoreAdj is the OOM score adjustment for Kubelet
	KubeletOOMScoreAdj int = -999
	// KubeProxyOOMScoreAdj is the OOM score adjustment for kube-proxy
	KubeProxyOOMScoreAdj  int = -999
	guaranteedOOMScoreAdj int = -997
	besteffortOOMScoreAdj int = 1000
```

From the oom score we know that BestEffort pods will be killed first. And Guaranteed pods
 have very low oom score which is unlikely to be choosed by oom killer. And the kubelet
 and kubeproxy have even higher priority than Guaranteed pods.

The score of Burstable pods are Guaranteed and BestEffort (4-999). Pods request more memory
 will have lower score.

You can check the detail computation from the qos policy source code.

### references
- [k8s official: QOS](https://kubernetes.io/docs/tasks/configure-pod-container/quality-service-pod/)
- [k8s source code: qos computation](https://github.com/kubernetes/kubernetes/blob/v1.19.0/pkg/apis/core/v1/helper/qos/qos.go)
- [k8s source code: qos policy](https://github.com/kubernetes/kubernetes/blob/v1.19.0/pkg/kubelet/qos/policy.go)
