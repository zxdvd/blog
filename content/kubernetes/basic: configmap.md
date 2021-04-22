```metadata
tags: kubernetes, configmap
```

## kubernetes: configmap

### create a configmap
You can create a configmap from a file.

    $ kubectl --kubeconfig xxx.kubeconfig create configmap -n dev \
        CONFIGMAP_NAME --from-file=PATH_OF_CONFIGMAP_FILE

By default, the basename of the file path becomes key while content is the file.
You can define the key by yourself using `--from-file=CUSTOMIZED_KEY=PATH_OF_FILE`.

From kubernetes 1.14, `kubectl` has built-in supports for `kustomize`. So you can also
 create or update configmap from a `kustomization.yaml`:

```yaml
configMapGenerator:
- name: cm-config
  files:
  - conf/config.js
  - prod_config_key=conf/config.prod.js
```

You can apply it using `kubectl apply -k .` at the folder with `kustomization.yaml`.
The name of the created configmap will have a suffix which is hash of the content.
This behavior can be disabled via options:

```yaml
generatorOptions:
  disableNameSuffixHash: true
```

### get configmap
It's easy to get a configmap via `kubectl get cm XXX -o yaml`. By this way, you get
 a yaml config file that holds all files in this configmap.

If you only want a specific file, you can copy it from container that mount it or
 using the `jsonpath` of output, like following:

    $ kubectl -n dev get cm XXX -o jsonpath='{.data.config\.js}' > config.js

### update a configmap
You cannot create the same configmap again. But you can use the `create --dry-run` to
 get the yaml config and then use `kubectl replace` to replace it:

    $ kubectl --kubeconfig xxx.kubeconfig -n dev create configmap CM_NAME \
        --from-file=PATH_OF_FILE -o yaml --dry-run \
        | kubectl --kubeconfig xxx.kubeconfig replace -n dev -f -

### mount a configmap
You need to map the configmap to a volume first at pod level. And then you can mount
 this volume to the container.

```yaml
---
apiVersion: v1
kind: Pod
metadata:
  name: demo1-pod
spec:
  restartPolicy: Never
  volumes:
    - name: cm-volume
      configMap:
        name: cm-demo-config
  containers:
    - name: demo1
      image: debian:10
      imagePullPolicy: Always
      command: ["sleep"]
      args: ["10000"]
      volumeMounts:
        - name: cm-volume
          mountPath: /app/demo/config
```

The container needn't to prepare the mount path. If exists, it will be overwritten.
 Each key in the configmap becomes a file under this path.

### mount a config file
Sometimes, you may want to map the configmap to a specific file but not folder in the
 container. You can mount with the `subPath` option. Change the `volumeMounts` to following:

```yaml
      volumeMounts:
        - name: cm-volume
          mountPath: /app/demo/config
          subPath: smart-school-config.js
```

By this way, the `/app/demo/config` becomes a config file.
Attention: the mounted file won't be refreshed when using `subPath`.

### refresh mounted files
You can update a configmap at any time, but will the mounted files receive the updates?
 If you use the `subPath` to mount a specific file, then it won't get updates. For other
 cases, you'll receive updates, but this is delay. You won't get the updates immediately.

### read write permissions
You'll get error if you try to write to the mounted configmap. Actually it is mounted
 as read-only file system, you can check it via `mount`. I also tried to remount with
 `-o remount,rw` but got a permission denied.

So you cannot update the mounted configmap. You may need other tools if you really need
 a updatable config center.

### references
- [kubernetes docs: configure pod configmap](https://kubernetes.io/docs/tasks/configure-pod-container/configure-pod-configmap/)
