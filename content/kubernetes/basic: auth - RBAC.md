```metadata
tags: kubernetes, user, auth
```

## kubernetes: RBAC authentication practice

You can read [kubernetes: authentication overview](./basic: user authentication overview.md)
 to have an outline of authentication.

After setup a kubernetes cluster, you may need to create and assign account for others.
It's easy to understand the RBAC authentication. But there is no way to create an account
 with a password like other systems.

Kubernetes uses service account and normal user to manage the cluster. Both of them have
 no password. In following section, I'll show how to create account to access the cluster
 using service account and normal user.

Let's create a test namespace at first:

    $ kubectl --kubeconfig xxxx.conf create ns test-namespace

### create service account
You can create service account via `kubectl` directly:

    $ kubectl --kubeconfig xxxx.conf create sa -n test-namespace test-user1

You can export the related yaml using the `--dry-run` option and `-o yaml` option:

    $ kubectl create sa -n test-namespace test-user1 --dry-run=client -o yaml

The output looks like following:

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  creationTimestamp: null
  name: test-user1
  namespace: test-namespace
```

You can also create a similar yaml config like and apply it:

    $ kubectl apply -f sa-test-user1.yaml

After created, you can list it:

```shell
$ kubectl -n test-namespace get sa
NAME         SECRETS   AGE
default      1         2m53s
test-user1   1         12s

$ kubectl -n test-namespace get sa test-user1 -o yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  creationTimestamp: "2021-02-06T09:16:32Z"
  name: test-user1
  namespace: test-namespace
  resourceVersion: "3662406"
  uid: 52c520b5-e287-4f7e-a94f-10b707a3bb89
secrets:
- name: test-user1-token-v54vd
```

Kubernetes creates a secret for this service account automatically. And can list and
 get details using kubectl:

```shell
$ kubectl -n test-namespace get secret
NAME                     TYPE                                  DATA   AGE
default-token-n4mtm      kubernetes.io/service-account-token   3      11m
test-user1-token-v54vd   kubernetes.io/service-account-token   3      9m14s

$ kubectl -n test-namespace get secret test-user1-token-v54vd -o yaml
apiVersion: v1
data:
  ca.crt: LS0tLS1CRUdJTiBDRVJUSUZJQ0FURS0tLS0tCk.....
  namespace: dGVzdC1uYW1lc3BhY2U=
  token: ZXlKaGJHY2lPaUpTVXpJMU5pSXNJbXRwWkNJNkl.....
kind: Secret
metadata:
  annotations:
    kubernetes.io/service-account.name: test-user1
    kubernetes.io/service-account.uid: 52c520b5-e287-4f7e-a94f-10b707a3bb89
  creationTimestamp: "2021-02-06T09:16:32Z"
  managedFields:
  - apiVersion: v1
    fieldsType: FieldsV1
    manager: kube-controller-manager
  name: test-user1-token-v54vd
  namespace: test-namespace
  resourceVersion: "3662405"
  uid: 1e7a2543-3ba1-40f1-b789-99f8623b0233
type: kubernetes.io/service-account-token
```

You can use the token in the `data` field to communicate with the cluster. The `ca.crt`
 is certificate of the cluster. All fields in `data` are base64 encoded.

    $ echo dGVzdC1uYW1lc3BhY2U= | base64 --decode         // got test-namespace

You can base64 decode the token and test it using curl, like:

    $ curl -k -vvv -H 'Authorization: Bearer <the base64 decoded token>' \
        'https://localhost:36666/api/v1/namespaces'

Don't forget the `Bearer ` before the token.

I got a 403 for the request, the rejected message is
 `namespaces is forbidden: User \"system:serviceaccount:test-namespace:test-user1\" cannot list resource \"namespaces\" in API group \"\" at the cluster scope`.

From the message, we know that it recognized the user but this user doesn't have
 permission of this api.

Then let's try with roles.

### create role
I'll create a role that can read pods of a namespace. Create a file role_read_pods.yaml
 with content like following (copied from official guide):

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: test-namespace   # you can delete this line and specify namespace when executing
  name: pod-reader
rules:
- apiGroups: [""] # "" indicates the core API group
  resources: ["pods"]
  verbs: ["get", "watch", "list"]
```

Let's apply it

    $ kubectl -n test-namespace apply -f role_read_pods.yaml

A role contains many rules and each rule defines allowed actions to resources.

### bind user and role
Now we have user and role. We can assign role to user via `RoleBinding`. Create file
 `test_role_bind.yaml` with following content:

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: rb-read-pods
  namespace: test-namespace
subjects:
- kind: ServiceAccount
  name: test-user1
  namespace: test-namespace
roleRef:
  # "roleRef" specifies the binding to a Role / ClusterRole
  kind: Role #this must be Role or ClusterRole
  name: pod-reader # this must match the name of the Role or ClusterRole you wish to bind to
  apiGroup: rbac.authorization.k8s.io
```

Let's apply it

    $ kubectl apply -f test_role_bind.yaml

Now user test-user1 and role pod-reader are bound. Let's try to list pods using `curl`

    $ curl -k -vvv -H 'Authorization: Bearer <the base64 decoded token>' \
        'https://localhost:36666/api/v1/namespaces/test-namespace/pods'

It returned an empty pod list since I had no pod running. It didn't return 403. The
 authentication is succeeded.

### cluster role and binding
You need to define `ClusterRole` for global objects like namespace. And with `ClusterRole`,
 you can define role to same kind of objects in **all** namespaces easily (you have to
 define same role in each namespace without `ClusterRole`).

Let's define a cluster role that can reads namespaces and pods in all namespaces.
 We can define multiple yaml configs in same file and apply them, just separate with
 `---`.

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  # "namespace" omitted since ClusterRoles are not namespaced
  name: cr-ns-and-pods
rules:
- apiGroups: [""]
  resources: ["namespaces", "pods"]
  verbs: ["get", "watch", "list"]

---

apiVersion: rbac.authorization.k8s.io/v1
# This cluster role binding allows anyone in the "manager" group to read secrets in any namespace.
kind: ClusterRoleBinding
metadata:
  name: crb-ns-and-pods
subjects:
- kind: ServiceAccount
  name: test-user1
  namespace: test-namespace
roleRef:
  kind: ClusterRole
  name: cr-ns-and-pods
  apiGroup: rbac.authorization.k8s.io
```

Let's apply it

    $ kubectl apply -f test_cluster_role.yaml

Then you can try to list all pods via `curl`:

    $ curl -k -vvv -H 'Authorization: Bearer <the base64 decoded token>' \
        'https://localhost:36666/api/v1/pods'

It will list pods in all namespaces since the user has the `ClusterRole` that has
 permission to pods.

Since a service account is also a normal user, you can also refer it in subjects of
 binding like following:

```yaml
subjects:
- kind: User
  name: system:serviceaccount:test-namespace:test-user1
```

Attention, kubernetes doesn't store user actually. So there is no `namespace` of
 user.

### create normal user
Above sections create a service account and bind with role. Now let's create a normal
 user and bind with role.

Unlike service account,  kubernetes doesn't store normal user. The cluster recognizes
 any client certificate signed by the cluster itself. The `CommonName` of the certificate
 will be used as username. And you can create role bindings to usernames or groups.

Let's create a RSA key at first:

    $ openssl genrsa -out user1.key 2048

And then create a certificate sign request (CSR):

    $ openssl req -new -key ./user1.key -out user1.csr -subj '/CN=user1/O=users:devops'

Now we have the csr file. We need to ask the kubernetes cluster to sign a certificate
 for it. We can submit a request via kubernetes api. Or, we can sign it directly if
 we can access the CA certificate and key.

#### sign using CA certificate directly
Let's sign the CSR directly using CA certificate and key:

    $ openssl x509 -req -in user1.csr \
        -CA /etc/kubernetes/pki/ca.crt -CAkey /etc/kubernetes/pki/ca.key \
        -CAcreateserial -days 3650 -out user1.crt

Now we have the signed client certificate and key. Let's test it using curl:

    $ curl -k -vvv --key ./user1.key --cert ./user1.crt  'https://localhost:36666/api/v1/namespaces/'

I got 403 and it indicated that it recognized the user but the user didn't have any
 permission.

Change the role binding config a little to contains following subject. So that user1
 has permissions.

```yaml
subjects:
- kind: User
  name: user1
```

Try again and I got expected result.

#### sign CSR via kubernetes api
For scenario that you cannot access kubernetes cluster CA key, you can submit CSR
 request via api.

Let's create key and CSR for another user user2. First get an one line base64 string
 of the CSR:

    $ cat user2.csr | base64 | tr -d '\n'

Then create a yaml config like csr_request.yaml:

```yaml
apiVersion: certificates.k8s.io/v1
kind: CertificateSigningRequest
metadata:
  name: user2
spec:
  groups:
    - users:devops
  request: <the_one_line_base64_CSR>
  signerName: kubernetes.io/kube-apiserver-client
  usages:
    - client auth
```

Let's apply this config via `kubectl`. And then list all CSR requests:

    $ kubectl get csr
    NAME    AGE   SIGNERNAME                            REQUESTOR          CONDITION
    user2   4s    kubernetes.io/kube-apiserver-client   kubernetes-admin   Pending

We can see the pending CSR request. Let's approve it and list CSR requests again:

```shell
$ kubectl certificate approve user2
certificatesigningrequest.certificates.k8s.io/user2 approved

$ kubectl get csr
NAME    AGE    SIGNERNAME                            REQUESTOR          CONDITION
user2   2m1s   kubernetes.io/kube-apiserver-client   kubernetes-admin   Approved,Issued
```

And then we can get the signed certificate via `kubectl get csr/user2 -o yaml`. The
 certificate is a one line base64 string at `status.certificate`. You need to decode
 it.

### kubectl config
You can test token or certificate using `curl` quickly. And you can create kubectl
 config file if there is no problem so that you can use kubectl without specifying
 auth file.

```shell
// set user and cluster
$ kubectl config set-credentials u1 --client-key ./user1.key --client-certificate ./user1.crt
$ kubectl config set-cluster prod --server=https://localhost:36666 \
        --certificate-authority /etc/kubernetes/pki/ca.crt

$ kubectl config set-context prod --cluster=prod --user=u1
$ kubectl config use-context prod              # set current context
```

Above command will setup kubectl config file at `$HOME/.kube/config`. You can also edit
 it manually.

For the `set-credentials` and `set-cluster`, you can add option `--embed-certs` so
 that it will base64 the certificate and embed it in the config file. Then the config
 file doesn't depend on any local file and you can send it to others or copy to other
 machines.

### validation
It seems that kubernetes doesn't check whether an object exists or not when applying
 configuration. I tried to bind role with an un-existed service account. It applies
 successfully.


### references
- [k8s official: RBAC authentication](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)
- [k8s official: api in one page](https://kubernetes.io/docs/reference/generated/kubernetes-api/v1.20/)
- [k8s official: certificate sign requests](https://kubernetes.io/docs/reference/access-authn-authz/certificate-signing-requests/)
