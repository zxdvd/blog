```metadata
tags: kubernetes, resource, limit, quota, memory cpu
```

## kubernetes resource manage: memory and cpu

Kubernetes collects capacities of memory, cpu and storage of each nodes and it can
 arrange containers properly according to their requirements. You can set request
 and limit of each container of a pod. You can also set total limit of a namespace.

For memory and cpu limit, it uses cgroups underground. You may find that you can
 choose systemd as the cgroup driver. But systemd also uses cgroups underground.


### limits and requests
For each container, there are two kinds of restrictions: `limits` and `requests`. So
 what's the difference?

`limits` is **hard** upper boundary. The container cannot use more resources than
 this limit.

While `requests` is more about basic requirement rather than restriction. By set
 this, you tell kubernetes that how much resources the container need to run properly.

And when scheduling a new pod, kubernetes will find a node and ensure that the node
 has enough resources according to `requests` of the pod (sum of each containers in
 the pod). For each node, kubernetes guarantees that the `requests` sum of all containers
 is under capacities of the nodes.

A example of pod settings:

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: frontend
spec:
  containers:
  - name: app
    image: images.my-company.example/app:v4
    resources:
      requests:
        memory: "64Mi"
        cpu: "250m"
      limits:
        memory: "128Mi"
        cpu: "500m"
  - name: log-aggregator
    image: images.my-company.example/log-aggregator:v6
    resources:
      requests:
        memory: "64Mi"
        cpu: "250m"
      limits:
        memory: "128Mi"
        cpu: "500m"
```

For cpu, `250m` means 250 millicpu, same to 0.25 vcpu and you can use `0.25` directly.

### default limits and requests
For each namespace, you can set a default limits and requests for each container. The
 default value is used if you didn't set it explicitly.

```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: mem-limit-range
spec:
  limits:
  - default:
      memory: 512Mi
      cpu: "1"
    defaultRequest:
      memory: 256Mi
      cpu: "0.5"
    max:
      memory: 1Gi
      cpu: "2"
    min:
      memory: 200Mi
      cpu: "0.5"
    type: Container
```

The `default` acts like `defaultLimit`. It will be the default `limits` of container.
 The `max` and `min` control the maximum limits and minimum requests when you specify
 it explicitly.

Attention, you can create multiple `LimitRange` for same namespace. Kubernetes doesn't
 prevent this and kubernetes won't check if there is conflict between them. You should
 match all these multiple `LimitRange` when creating new pods. If there is conflict,
 you may not create pod successfully.

### resource quota
Kubernetes supports set cpu and memory quotas of each namespace. With this, we can
 easily assign resources to different teams. You can setup namepace for each team
 and set quotas accordingly. By this way, if one team deploys bug code that eats
 all memory, it won't affect other teams.

A demo quotas setting from official document:

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: mem-cpu-demo
spec:
  hard:
    requests.cpu: "1"
    requests.memory: 1Gi
    limits.cpu: "2"
    limits.memory: 2Gi
    pods: "5"
```

The `pods` quota limits the total pods in a namespace.

If you set `ResourceQuota` for a namespace, then each container in this namespace must
 set related limits. And kubernetes guarantees that total resources of all containers
 won't exceed the quotas.


### references
- [k8s official: manage resources](https://kubernetes.io/docs/concepts/configuration/manage-resources-containers/)
- [k8s official: resource quota](https://kubernetes.io/docs/tasks/administer-cluster/manage-resources/quota-memory-cpu-namespace/)
