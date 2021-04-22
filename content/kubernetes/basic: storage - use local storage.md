```metadata
tags: kubernetes, storage, volume
```

## kubernetes storage: use storage of local machine

I think running stateless pod is the most frequent scenario. Most stateful data
 was stored in database, oss, configs can be stored using configmap. It's easy
 to scale stateless applications. You can deploy them to any nodes.

But for infrastructure teams, they need to maintain stateful services like mysql,
redis, distributed file systems. Stateful pods need to bind with PVC. For some
 applications, you can use remote storage like nfs so that they don't need to bind
 with specific nodes. But if you want fast io, you may still need storage from local
 machine.

### use hostPath
You can use `hostPath` to mount a file or directory from host to pod. The problem
 with `hostPath` is that there is no strong dependency between the mounted `hostPath`
 volume and your pod. Your pod may be migrated to another node and them it lost
 connection with previous data. You may need to use other method like `nodeName` to
 bind pod with current using node.

For some use cases, like log storage, pod needs not to care about previous stored
 data. You don't need to care about above problem and you can use `hostPath` easily.
 Actually, we mount a directory into each pod and write logs into that directory. And
 a DaemonSet pod will collect all logs from that directory at each node. It works very
 well.

For applications like mysql, postgres, you really need to take care about `hostPath`.

### local persistent volume
Local persistent volume is the preferred way for most time. The
 [official blog](https://kubernetes.io/blog/2019/04/04/kubernetes-1.14-local-persistent-volumes-ga/)
  explains the difference between local persistent volume and hostPath clearly:

    The biggest difference is that the Kubernetes scheduler understands which node a Local
    Persistent Volume belongs to. With HostPath volumes, a pod referencing a HostPath volume
    may be moved by the scheduler to a different node resulting in data loss. But with Local
    Persistent Volumes, the Kubernetes scheduler ensures that a pod using a Local Persistent
    Volume is always scheduled to the same node.

The pod is bound with the PV so that it is bound with the node that provides the PV.

Create a StorageClass at first.

```yaml
kind: StorageClass
apiVersion: storage.k8s.io/v1
metadata:
  name: local-storage
provisioner: kubernetes.io/no-provisioner
volumeBindingMode: WaitForFirstConsumer
```

Then create PV that binds with the storage class.

```yaml
apiVersion: v1
kind: PersistentVolume
metadata:
  name: example-local-pv
spec:
  capacity:
    storage: 500Gi
  accessModes:
  - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: local-storage
  local:
    path: /mnt/disks/vol1
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: kubernetes.io/hostname
          operator: In
          values:
          - my-node
```

And then set `volumeClaimTemplates` in StatefulSet deployment to claim volume from the
 storage class.

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: local-test
spec:
  serviceName: "local-service"
  template:
    spec:
      containers:
      ...
  volumeClaimTemplates:
  - metadata:
      name: local-vol
    spec:
      accessModes: [ "ReadWriteOnce" ]
      storageClassName: "local-storage"
      resources:
        requests:
          storage: 368Gi
```

### references
- [k8s official: storage volumes](https://kubernetes.io/docs/concepts/storage/volumes/)
- [k8s blog: local PV GA](https://kubernetes.io/blog/2019/04/04/kubernetes-1.14-local-persistent-volumes-ga/)
- [k8s blog: local PV beta](https://kubernetes.io/blog/2018/04/13/local-persistent-volumes-beta/)
