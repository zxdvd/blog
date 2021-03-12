```metadata
tags: vpn, wireguard
```

## vpn: wireguard

`wireguard` is a easy, simple and fast VPN and it has been included in the linux kernel
 since linux 5.6. It's fast since it works in kernel space so that it can avoid much more
 unnecessary context switches.

You may want to use it to escape network restriction of some countries but it's not good
 at this. You may need tools that supports traffic obfuscating.

### server setup
You still need the userspace tool even though you have a kernel >= 5.6.

    $ sudo apt install wireguard-tools

Generate a pair of keys:

    $ wg genkey > wg.key            # private key
    $ wg pubkey < wg.key > wg.pub   # get public key from the private key

Write a config file like `/etc/wireguard/wg0.conf` as following:

```
[Interface]
ListenPort = 40000
PrivateKey = 6EtwgBRCCKcZaawmWOtHlO0PvN1L0zf6fZt4fePA/mM=
```

The `PrivateKey` is content of the generated private key. The the server will listen UDP
 connections on port 40000 (curretly only UDP is supported). Remeber to config firewall
 to allow traffic on this port.

Then you can setup the TUN device and address and then apply the config using
 `wg setconf DEV_NAME /etc/wireguard/wg0.conf`. Or using the `wg-quick` tool like:

    $ systemctl start wg-quick@wg0.service

### client setup
Generate a pair of keys just like the server side. And setup the config file like following:

```
[Interface]
PrivateKey = MAaEUKAck7ZZJk5XdHHCSmQretCZFD5o0zoxXM86WWQ=

[Peer]
PublicKey = qKjY/8rVP+4ym7UL/S9/hoKLA4T1gLPXP8YpWwuFfEo=
Endpoint = 1.1.1.1:40000
AllowedIPs = 0.0.0.0/0
```

The **Interface** section configs the local side while the *Peer** section configs the remote
 side. The **Interface** with a `ListenPort` acts as server. And to connect to the server,
 the client sets server's host and port via `Endpoint`. The `PublicKey` in the **Peer** is
 the public key of the remote side.

Then apply the config at client side. And client side is finished.

### add client to server
The client has configed to connect to the server and authenticated using server's public key.
The server also needs to add a **Peer** section of the client.

Add following to config file of the server:

```
[Peer]
PublicKey = nZMtS2pwOjTFmS3/m1lFiFc3qTthOkL6odi/z5pMOB4=
```

The `PublicKey` is public key of the client. And apply the changes.

### macos setup
Linux has in-kernel support of wireguard while other OSes don't. So how to use it on macos?
 `wireguard` has userspace implementation: `wireguard-go`. We can use it to setup a TUN
 device and communicate using the wireguard protocol. You still need the `wireguard-tools`
 to deal with config.

You may try to setup ip address of the TUN device like following:

    $ sudo ifconfig utun8 192.168.166.10/24
    ifconfig: ioctl (SIOCAIFADDR): Destination address required

And you'll get error, you need to specify the remote ip address while setting ip for a TUN device:

    $ sudo ifconfig utun8 192.168.166.10 192.168.166.1 up

### faq
#### error: Line unrecognized: Address=XXXXX
You may get above error when you did a `wg setconf wg0 /etc/wireguard/wg0.conf` after setting
`Address=xxxx` in the **Interface** section following some tutorials.

Actually, `Address=xxx` and `SaveConfig=true` are only supported by tool `wg-quick`. `wg`
 doesn't support them. If you preferred `wg setconf`, you should not use them in the config
 file.


### references
- [archwiki: wireguard](https://wiki.archlinux.org/index.php/WireGuard)
- [wireguard official site](https://www.wireguard.com/)
- [userspace implementation: wireguard in go](https://github.com/WireGuard/wireguard-go)
