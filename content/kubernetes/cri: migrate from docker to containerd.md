```metadata
tags: kubernetes, cri, docker, containerd
```

## kubernetes cri: migrate from docker to containerd

Kubernetes implements CRI (container runtime interface) so that any container runtime
 can be used in kubernetes if it implements the CRI interface.

However, docker didn't implement CRI then kubernetes team needs to maintain the docker
 shim which acts as translator between CRI and docker daemon. The kubernetes team don't
 want to maintain it and they plan to remove docker support in the future. That's why
 you should consider migrating to other container runtime, like containerd.

Docker also uses containerd underground. So if you have docker installed, you already
 have containerd.

To migrate to containerd, we need to stop the cluster, change some configs and restart
 the cluster.

### stop the cluster
Drain the node

    $ kubectl drain iz2ze4060zphcpsld7dltyz  --ignore-daemonsets

Stop kubelet

    $ systemctl stop kubelet.service

### prepare containerd
Since docker depends on containerd, you have had containerd installed. You can check
 it using `ctr` (the cli tool of containerd)

```shell
$ ctr version
Client:
  Version:  1.4.3
  Revision: 269548fa27e0089a8b8278fc4fc781d7f65a939b
  Go version: go1.13.15

Server:
  Version:  1.4.3
  Revision: 269548fa27e0089a8b8278fc4fc781d7f65a939b
  UUID: b084a74b-2db2-4130-9730-684a2d5020cc
```

Generate a default config (you may need to backup the old one):

    $ containerd config default | tee /etc/containerd/config.toml

For system that uses systemd, it is suggested to set systemd as cgroup driver.
Change the `config.toml` to have following:

    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
        SystemdCgroup = true

Restart the containerd

    $ systemctl restart containerd

Then we need to tell kubelet to use containerd as CRI. Edit file
 `/var/lib/kubelet/kubeadm-flags.env` and add following two options:

     --container-runtime=remote --container-runtime-endpoint=unix:///run/containerd/containerd.sock

If you have choose to use `systemd` as cgroup driver, then you also need to add
 option `--cgroup-driver=systemd` to above kubeadm flags file.

Then start kubelet

    $ systemctl start kubelet


to un-drain the node

    $ kubectl uncordon iz2ze4060zphcpsld7dltyz

### mirror registry
We may have set registry mirrors due to all kinds of reasons (local cache or network
 blocking). Then we need to set registry mirrors for containerd too since it's containerd
 pulling the images.

We can add registry mirror in `config.toml` of containerd. You can have many mirror settings.
 It ships one for `docker.io` by default.

```ini
    [plugins."io.containerd.grpc.v1.cri".registry]
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
        [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
          endpoint = ["https://registry-1.docker.io"]
        [plugins."io.containerd.grpc.v1.cri".registry.mirrors."k8s.gcr.io"]
          endpoint = ["https://registry.aliyuncs.com/google_containers"]
```

And don't forget to change the sandbox_image. It's `k8s.gcr.io/pause:3.2` on my server.
I need to change it to `registry.aliyuncs.com/google_containers/pause:3.2`.

### can container and docker co-exist?
We know that docker also uses containerd underground. So we may wonder that could them
 co-exist with each other?

Yes, they could. You don't need to stop or remove docker. Containerd listens on
 `/var/run/containerd/containerd.sock` while docker listens on `/var/run/docker.sock`.

And containerd supports isolated via namespace. Docker uses namespace `moby` while
 kubernetes uses namespace `k8s.io`. They are separated. You can use `ctr ns ls` to
 list all namespaces. To list all containers ran by kubernetes, using

    $ ctr -n k8s.io c ls

### references
- [k8s official: container runtime setup](https://kubernetes.io/docs/setup/production-environment/container-runtimes/)
- [containerd issue: mean of registry.mirrors](https://github.com/containerd/cri/issues/1138)
