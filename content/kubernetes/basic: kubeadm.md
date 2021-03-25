```metadata
tags: kubernetes, cluster
```

## kubernetes: setup via kubeadm

#### requirements
You need an ordinary linux server with more than 2GB RAM. And you'd better disable
 swap. And choose a proper container runtime to install.

### install kubeadm kuebelet kubectl
You need to install kubeadm, kuebelet and kubectl by yourself. There are official
 repositories for ubuntu, debian and redhat linux.

You may find that the repositories are too old. No support for debian buster and
 ubuntu 18.04 and above. It's ok to use repository `kubernetes-xenial`. Kubernetes
 is written with go and it doesn't have other dependencies.

### init
The default image repository is `k8s.gcr.io`. You can change it to other repository.
 It will download images for etcd, coredns, kube-scheduler, kube-controller-manager,
 kube-apiserver and kube-proxy.

    # kubeadm init --apiserver-bind-port 36666 --image-repository registry.aliyuncs.com/google_containers

If the master has multiple IPs, or you'll access it via DNS names, you should add
 following options to the `kubeadm init`

    # kubeadm init ......... --apiserver-cert-extra-sans x.x.x.x,k8s.a.com

Otherwise, you'll get certificate error when trying to access the cluster via other names.

You can also init from a config file. You can get a default config via following command:

    # kubeadm config print init-defaults

You can pipe output to a file and edit it and then apply it using

    $ kubeadm init --config <THE_CONFIG_FILE>

A demo config file:

```yaml
apiVersion: kubeadm.k8s.io/v1beta2
kind: InitConfiguration
localAPIEndpoint:
  advertiseAddress: 0.0.0.0
  bindPort: 36666
---
apiVersion: kubeadm.k8s.io/v1beta2
kind: ClusterConfiguration
apiServer:
  timeoutForControlPlane: 4m0s
  certSANs:
  - 127.0.0.1
  - localhost
  - k8s.example.com
clusterName: kubernetes
controllerManager: {}
dns:
  type: CoreDNS
etcd:
  local:
    dataDir: /var/lib/etcd
imageRepository: registry.aliyuncs.com/google_containers
kubernetesVersion: v1.20.0
networking:
  dnsDomain: cluster.local
  serviceSubnet: 10.96.0.0/12
  podSubnet: 10.244.0.0/16
```

I added some SANs, changed the apiserver port and imageRepository in above config.

### ready to use
Then the kubernetes server is ready to use.

    # kubectl --kubeconfig /etc/kubernetes/admin.conf -n all get ns       // get all namespaces
    # kubectl --kubeconfig /etc/kubernetes/admin.conf -n all get nodes    // get all nodes

### network
There are too many CNIs. I'd preferred calico. It's easy to install using the `tigera-operator`.

### reset whole cluster
You can use `kubeadm reset` to reset whole cluster. It will destroy all running components
 , containers and key pairs.

### references
- [k8s official: install kubeadm](https://v1-18.docs.kubernetes.io/docs/setup/production-environment/tools/kubeadm/install-kubeadm/)
- [k8s official: kubeadm init](https://kubernetes.io/docs/reference/setup-tools/kubeadm/kubeadm-init/)
- [calico](https://docs.projectcalico.org/getting-started/kubernetes/quickstart)
