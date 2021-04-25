```metadata
tags: network, iptables, load-balance
```

## network: iptables - load balance via statistic module

From article
[network: iptables port forwarding](./network: iptables port forwarding.md)
 we know that it's easy to do port forwarding using iptables.

Then you may wonder that is it possible to use it as load balancer? May I configure
 iptables to forward to multiple backends?

Is it possible to add multiple DNAT backends like following?

    $ iptables -t nat -I PREROUTING  -p tcp  --dport 80 \
        -j DNAT --to-destination 192.168.1.10:8080
    $ iptables -t nat -I PREROUTING  -p tcp  --dport 80 \
        -j DNAT --to-destination 192.168.1.11:8080
    $ iptables -t nat -I PREROUTING  -p tcp  --dport 80 \
        -j DNAT --to-destination 192.168.1.12:8080

Actually, you can add these rules without problem. But it won't work since iptables
 finds the first matched rule and applies it sequencially. All following rules are
 ingored.

### statistic extension
Iptables has a `statistic` extention. You can apply it on a rule so that you can set
 a probability that it will be matched. Like following

    $ iptables -t nat -I PREROUTING  -p tcp  --dport 80 \
        -m statistic --mode random --probability 0.33   \
        -j DNAT --to-destination 192.168.1.10:8080

It means that if a packet matched with the rule, iptables adds a addtional probability
 to the final matching result. For above example, there is 33% probability that this
 rule is applied. There is 67% probability that it is ignored and iptables will try
 with following rules.

The pseudo code looks like following:

```js
    if (rule_matched) {
        const rand = Math.random()
        if (rand <= 0.33) {
            apply_current_rule()
            return
        }
        try_next_rule()
    }
```

Attention, if you want to proxy to 3 backends evenly. The probability of the first rule
 is 0.33 so that it will get 33% traffic.

However, for the second rule, the probability must be 0.50. Since there is 1-0.33 traffic
 will get to the second rule. Set probability as 0.5 so that the second rule will get 33%
 of total traffic.

For the third rule, no need to add probability. Just leave all left traffic to it.

The `statistic` extention also supports another mode `--mode nth` that you can use it to
 choose one packet from every N packets.

### real world cases
The `kube-proxy` component of `kubernetes` proxies traffic from service to pods. And it
 uses iptables underground. We know that a service may bind with many pods. And it uses
 the statistic extension to do load balance between multiple pods.

### references
- [blog: iptables load balance](https://scalingo.com/blog/iptables)
- [man page: iptables extensions](https://ipset.netfilter.org/iptables-extensions.man.html)
