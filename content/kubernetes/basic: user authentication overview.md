```metadata
tags: kubernetes, user, auth
```

## kubernetes: user authentication overview

Each component of kubernetes needs authenticating while communicates with each other.
 And users need to authenticate to mange the clusters.

Kubernetes supports a lot of different kinds of authenticating methods. The role based
 access control (RBAC) is a popular method and is the default one used by kubeadm and
 many other tools.

### RBAC
RBAC is a very common design in many management systems. Often following objects are
 referred: resources, roles, users, user-role bindings. You have a lot of resources to
 manage. A role is a set of rules that defines what kinds of actions this role has to
 one or more resources. You can define some roles and users. And then binding roles to
 each user.

For kubernetes, a cluster has a lot of resources like: pods, services, nodes, configmaps
 and many others. You can define a monitoring role that can only get or list resources
 like pods, services, a deploy role that can get/list/create/update resources. Then you
 can assign role to different users.

Kubernetes resource (object) can be namespaced like pods, deployments or globally like
 nodes, namespaces. Kubernetes uses two separate role for them: `Role` for namespaced
 objects, `ClusterRole` for global objects. Attention, `ClusterRole` can also contain
 namespaced object like pods, then it means that this cluster role has access to pods in
 all namespaces.

Corresponding to the two roles, there are two binding objects: `RoleBinding` for `Role`
 and `ClusterRoleBinding` for `ClusterRole`.

A role binding can bind a role and one or multiple subjects. A subject can be a normal
 user, an user group or a service account.

Service account itself is also a normal namespaced object. You can create it via `kubectl`
 command or api. And kubernetes will also create a secret that will store token of this
 service account at the same time. Then uou can use this token to authenticate.

 However, things are different for user and group. Kubernetes doesn't have any internal
 object to store user or group. To create an user, you should generate a certificate
 and ask the cluster to approve ans sign the certificate. Then you can use the signed
 certificate to communicate with the cluster. But what's the user and group?  The
 `CommonName (CN)` of the certificate acts as username while `Organization (O)` acts
 as group. By this way, kubernetes needn't to store users and groups. But the bad side
 are:

    1. you cannot revoke an issued certificate
    2. no way to show all issued certificates

To avoid the problems, it is suggested to bind with user but not group. By this way,
 you can get all bound users. For group, you don't know how many users have been bound
 under this group.

For each service account, it also has an username like
 `system:serviceaccount:<namespace>:<sa_name>`. So you have two ways to refer a service
 account: via serviceaccount and via user.

### references
- [k8s official: authentication](https://kubernetes.io/docs/reference/access-authn-authz/authentication/)
- [k8s official: RBAC authentication](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)
