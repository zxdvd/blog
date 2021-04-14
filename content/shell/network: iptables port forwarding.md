```metadata
tags: network, iptables
```

## network: iptables port forwarding

Following image is packet flow in tables and chains in kernel:

![iptables packet flow](./images/iptables.png)

### port forwarding
A general use case is port forwarding. You can forward all traffics to port 80 to
 another server. The traffic flow is like following:

    client   ->  gateway 192.168.1.1:80    ->  backend 192.168.1.10:8080

Set following iptables rules to achieve the port forwarding:

    gateway $ iptables -t nat -I PREROUTING  -p tcp  --dport 80 \
        -j DNAT --to-destination 192.168.1.10:8080 -m comment --comment 'forward 80'

This will change destination of matched incoming packets to another server before routing.

The backend server gets forwarded packets and replies with source ip of backend server
 to client directly. Direct returning needs more config and gateway support. Otherwise
 client cannot recognize this and may reject it.

To avoid this and make things simple, we can do a SNAT at gateway to change source ip
 of packets so that the returning packages will also go through the gateway.

    gateway $ iptables -t nat -I POSTROUTING -d 192.168.1.10 -p tcp --dport 8080 \
        -j SNAT --to-source 192.168.1.1 -m comment --comment 'forward 80'

With above two iptables, it acts as a simple TCP proxy. You can also achieve it using
 nginx or haproxy. But the iptables way is simple and it has better performance since
 it is in-kernel forwarding.

#### allow forwarding
If above not working, you need to check if port forwarding is enabled or not. And also
 check the `FORWARD` chain.

`cat /proc/sys/net/ipv4/ip_forward` to check if forwarding is enabled. `echo 1` to that
 if it is not. It is enable in most distributions by default.

You can check the iptables `FORWARD` chain via `iptables -v -L FORWARD`.

```shell
# iptables -v -L FORWARD
Chain FORWARD (policy DROP 0 packets, 0 bytes)
 pkts bytes target     prot opt in     out     source               destination
  10M  461M KUBE-FORWARD  all  --  any    any     anywhere             anywhere             /* kubernetes forwarding rules */
  10M  461M KUBE-SERVICES  all  --  any    any     anywhere             anywhere             ctstate NEW /* kubernetes service portals */
  10M  461M KUBE-EXTERNAL-SERVICES  all  --  any    any     anywhere             anywhere             ctstate NEW /* kubernetes externally-visible service portals */
  10M  461M DOCKER-USER  all  --  any    any     anywhere             anywhere
```

In above example, the default policy for `FORWARD` chain is `DROP`. So if a packet doesn't
 match any rule in this chain, it is dropped.

We can set default policy to `ACCEPT` using `iptables --policy FORWARD ACCEPT`.

If you think allow all is too relax, you can using following to allow a specific port

    gateway$ iptables -I FORWARD -p tcp -d 192.168.1.10 --dport 8080 -j ACCEPT

Attention, the ip and port of target server is used here. You may think that `port 80` is
 the destination. However, remember that `FORWARD` chain effects after the `PREROUTING`
 chain and the destination has been changed to final target in the `PREROUTING` chain. So
 you need to use ip and port of final target at here.

#### allow local traffic
After setup port forwarding, you may find that it works well from other servers. But when
 you tried to access port 80 from the gateway itself, it doesn't forward to target backend
 server.

Just look carefully of the iptables flow image, we can find that traffics from local processes
 `DO NOT` go through the `PREROUTING` chain. It goes to the `OUTPUT` chain at first and
 then to the `POSTROUTING` chain directly.

So the `DNAT` in `PREROUTING` won't work. Then we know that the forwarding won't work for
 local processes. But we can tackle this by set `DNAT` in `OUTPUT` chain. Like following:

    gateway$ iptables -t nat -I OUTPUT -p tcp  --dport 80 -j DNAT --to-destination 192.168.1.10:8000

Then traffics from both outside and the gateway will work.

### references
- [blog: depth guide iptables](https://www.booleanworld.com/depth-guide-iptables-linux-firewall/)
