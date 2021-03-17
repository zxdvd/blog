```metadata
tags: kubernetes, setup, certificate
```

## kubernetes: update certificates with extra SANs

I got following error when running `kubectl get ns`:

    Unable to connect to the server: x509: certificate is valid for ld7dltyz, kubernetes,
    kubernetes.default, kubernetes.default.svc, kubernetes.default.svc.cluster.local, not localhost

I found a lot of people have got this problem. The reason is that client tries to connect
 to the cluster using IP or domain that not matched with SANs of certificate of kubernetes
 api server.

I set up the cluster via kubeadm. It adds my two IPs and domain `kubernetes`, `kubernetes.default`
 and others into the SANs of the certificate. It seems that it didn't add `localhost`. My
 public elastic IP is also not included. So I got above error while using localhost or
 public IP.

We can skip this error easily via the `--insecure-skip-tls-verify` option when using
 kubectl, or `curl --insecure/-k` so that it will skip the host checking.

However, it is not safe and you'd not use this to access production cluster. To avoid
 it, we should add all IPs and domains that are allowed to access the cluster to SANs.

During the setup stage, you can add them like following:

    # kubeadm init ......... --apiserver-cert-extra-sans x.x.x.x,k8s.a.com

But, what about IP or domain is changed after the cluster was set up? How to renew
 the certificates with extra SANs?

Firstly, get the `kubeadm` config from the cluster.

    // the kubeadm config view is deprecated
    $ kubectl get cm -n kube-system kubeadm-config \
            -o jsonpath='{.data.ClusterConfiguration}' > kubeadm.yml

Then add SANs to the `certSANs` field of `apiServer`, like following:

```yaml
apiServer:
  certSANs:
    - "localhost"
    - "127.0.0.1"
    - "k8s.example.com"
  extraArgs:
    authorization-mode: Node,RBAC
```

Then backup the `/etc/kubernetes/pki` directory and remove the old key and certificate
 (it will reuse old one if existed) and generate new certificates:

    $ cd /etc/kubernetes/pki && rm apiserver.key apiserver.crt
    $ kubeadm -v 10000 init phase certs apiserver --config ./kubeadm.yml

It should generate new certificates. We can check it using `openssl x509 -text -in xxx.crt`.

The X509v3 extensions should contains SANs like following:

    X509v3 Subject Alternative Name:
                   DNS:ld7dltyz, DNS:k8s.example.com, DNS:kubernetes, DNS:kubernetes.default,
                   DNS:kubernetes.default.svc, DNS:kubernetes.default.svc.cluster.local,
                   DNS:localhost, IP Address:10.96.0.1, IP Address:127.0.0.1

Now we can restart the apiserver so that it will load the new key and certificate.
Just kill the process and restart kuebelet.

Try to connect to the cluster using newly added IP or domain, it should be ok.

Finally, we should update the `kubeadm-config` configmap in `kube-system` with the changes.
So that `kubeadm upgrade` knows the changes and will apply them during upgrade.

Some articles suggest to use `kubeadm config upload from-file` to update, but it seems that
 it only works with kubeadm 1.7 and below.

### references
- [kubeadm issue: update api server certSANs](https://github.com/kubernetes/kubeadm/issues/1447)

