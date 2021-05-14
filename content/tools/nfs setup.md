```metadata
tags: server, nfs
```
## nfs setup

This article is tested with nfs4 only. There is a lot of differences if you are using
 nfs3.

### server side
Create an export directory (you can use any name you want):

    $ mkdir /exports
    $ mkdir /exports/frontends

Bind mount the to-be-exported directory to target mount point.

    $ mount --bind /mnt/frontends /exports/frontends

You need to add this in `/etc/fstab` so that it will mount while system booting.

```text
# add in /etc/fstab
/mnt/frontends    /exports/frontends    none    bind     0    0
```

Export the directory by add a line in /etc/exports like following:

```text
/exports 172.16.0.0/24(rw,sync,fsid=0,no_subtree_check)
/exports/frontends 172.16.0.0/24(rw,sync,no_subtree_check)
```

And then export it using `exportfs -a` command. Each time you changed the `/etc/exports`,
 you need to refresh the export via `exportfs -ar`.

### client side
You can use `showmount  -e 172.16.0.1` to list all exported endpoints of the server.
You can mount an endpoint using mount command like following:

    $ mkdir /imports/frontends
    $ mount -t nfs4 172.16.0.1:/frontends /imports/frontends

Attention, it is `/frontends` but not `/exports/frontends` at here. This is different
 from nfs3.

If everything looks ok, you should set it in `/etc/fstab` so that it will mount at
 startup.

```text
172.16.0.1:/frontends         /mnt/frontends        nfs4    defaults    0    0
```

### exports options
#### fsid=0
For nfs4, you need to add `fsid=0` for the root export directory.


### mount options
#### soft vs hard
Simply speaking, `hard` is safe, just like local disk. Processes will hang and wait if
 there is problems that lead to io fail. While `soft` has a fast responsiveness but there
 may have buffered data in kernel that has not been flushed to nfs and this will lead to
 data loss when nfs disconnected.
