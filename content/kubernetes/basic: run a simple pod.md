```metadata
tags: kubernetes, cli
```

## run a simple pod

Write a simple yaml config like following:

```yaml
---
apiVersion: v1
kind: Pod
metadata:
  name: demo1-pod
spec:
  containers:
    - name: demo1
      image: debian:10
      imagePullPolicy: Always
      command: ["echo"]
      args: ["hello", "world"]
  restartPolicy: Never
```

And you can create this simple pod using `kubectl`:

    $ kubectl --kubeconfig xxx.kubeconfig create --namespace dev -f pod.yml

You can delete it using `kubectl`:

    $ kubectl --kubeconfig xxx.kubeconfig delete --namespace dev -f pod.yml

### debug interactively
You can run the pod using command like `sleep 1000` and then use `kubectl exec`
 to run commands or debug interactively.

```yaml
      image: debian:10
      command: ["sleep"]
      args: ["1000"]
```

    $ kubectl --kubeconfig xxx.kubeconfig --namespace dev exec demo1-pod -c demo1 -- echo dfdfdd

If your pod has only one container, you can ignore the `-c container_name`:

    $ kubectl --kubeconfig xxx.kubeconfig --namespace dev exec demo1-pod -- echo dfdfdd

You can also get an interactive shell using `exec -it`:

    $ kubectl --kubeconfig xxx.kubeconfig --namespace dev exec -it demo1-pod -- /bin/bash

### references
- [kubernetes command reference](https://kubernetes.io/docs/reference/generated/kubectl/kubectl-commands#create)
