+++
title = "Linux--multiple routing tables"
date = "2014-10-30"
categories = "Linux"
tags = ["linux", "network"]
slug = "multiple-routing-tables"
Authors = "Xudong"
+++

Several months ago, my colleague got a problem that some of his virtual
machines. I dug into this problem and finally solved it.

Following is the network topology =

![network topology](/images/post/network-topo-mult-routes.jpg)

It has following problems =   
The br2 and vnet5 and vnet6 can only communicate with subnet 192.168.2.0/24,
they cannot send or receive packages from other subnets.

Explanation =   
Because there is only route to 192.168.2.0/24 for the br2. Let's try to analyze
it through a ping process. If you ping 192.168.2.7 (br2) from a machine outside
of this subnet, like 192.168.1.10, below is the detailed routing path =    
src to dst = 192.168.1.10 checks the destination and finds it's in another subnet
so send it to the default gateway.  

    192.168.1.10  -->  to gateway 192.168.1.254   OK
    192.168.1.254 -->  forward to 192.168.2.254   OK
    192.168.2.254 -->  forward to 192.168.2.7     OK

dst to src = Also 192.168.2.7 would send to default gateway, but let's check the
default gateway of the machine. It's `default via 192.168.1.254 dev br1`, so it
fails to send the package since the br1 interface won't send a package whose
source ip address is not bounded to itself.

Now we knows that the br2 can receive packages but fail to send them to other
subnets. So how to? Add another default gateway? A machine can only have one
default gateway.

<!--more-->

### Routing tables

It's time to get something about the routing tables. For the linux, you can have
routing tables for 0 to 255. And you can set rules of each table, e.g. let the
br1 follows the table 10 and the br2 follows the table 20, with this, our
problem could be tackled perfectly.

All routing tables are listed in file `/etc/iproute2/rt_tables`, you can see
tables local, main, default there, main is the default table when you do all
kinds of add/delete.   
To check items in a table, using `#ip route show table TABLE-ID-or-TABLE-NAME`

### Setup multiple routing tables

From above we know, to solve this problem we need to set up  routing tables for
both br1 and br2. We can set the br1 to use the main table by default then we
could only set up another one for the br2.

1. `#echo 10 table_br2 >> /etc/iproute2/rt_tables` to add a table.

2. `#ip route add 192.168.2.0/24 dev br2 src 192.168.2.7 table table_br2`   
   `#ip route add default via 192.168.2.254 dev br2 table table_br2`

3. After setting up this table, we need to rules to let the packages to know
   which table to select.    
   `#ip rule add from 192.168.2.7 table table_br2`   
   This rule tells all packages whose source address is 192.168.2.7 to follow
   the table_br2.   
   `#ip rule add oif br2 table table_br2    //oif=output interface`

Now all the settings are finished. But remember that =

**All the `ip XXX` settings are temporary** which mean that they'll get lost
   after rebooting. If you want to keep them permanent, you can write them to
   "/etc/init.d/boot.local" or use other scripts.


#### References

1. [routing tables](http =//linux-ip.net/html/routing-tables.html)

2. [routing--multiple uplinks](http =//lartc.org/howto/lartc.rpdb.multiple-links.html)
