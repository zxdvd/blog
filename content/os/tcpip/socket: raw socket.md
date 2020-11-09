```metadata
tags: linux, network, tcp, socket
```

## tcpip: raw socket

While doing network programming, we only need to create socket, send and receive data.
We know that IP, TCP and UDP have headers but we don't need to care about them since
 OS mange them well. We only handle payload data.

In some situations, like sniffing packets, we may need both data and headers, thus
 raw socket data.

You can capture IP raw socket (L3) or link layer raw socket (L2). For L3 raw socket,
 you can manipulate IP header and payload. If it's a TCP socket, then the payload is
 TCP header and data.

For L3 raw socket, OS doesn't know which process the packet dispatches to since the L4
 layer is not parsed. It means that you'll receive all IP packets that were sent to
 this machine.


### a simple demo
Following is a simple python script that parses ip address and port from IP header and
 tcp header of all received TCP packets.

```python
    s = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_TCP)
    while True:
        r1 = s.recvfrom(1500)
        ip_raw = r1[0]
        ip_header = ip_raw[:20]
        ip_header_fmt = [
            '!',      # network order, big endian
            '12B',    # first 12 bytes
            '4B',     # 4 bytes source ip
            '4B',     # 4 bytes target ip
        ]

        ip_header = struct.unpack(''.join(ip_header_fmt), ip_header)
        source_ip = ip_header[-8:-4]    # second last 4 bytes
        dest_ip = ip_header[-4:]        # last 4 bytes

        tcp = ip_raw[20:]
        tcp_header = tcp[0:20]
        tcp_data = tcp[20:]
        tcp_header_fmt = [
            '!',       # network order, big endian
            'H',       # source port
            'H',       # dest port
            '16B',     # other 16 bytes in tcp header
        ]
        tcp_header = struct.unpack(''.join(tcp_header_fmt), tcp_header)
        source_port = tcp_header[0]
        dest_port = tcp_header[1]
        print('source : %s, %s' % (str(source_ip), source_port))
        print('dest: %s, %s' % (str(dest_ip), dest_port))
```

### raw socket protocol
You can sniff other protocols like UDP, ICMP and even ARP protocol.

```python
    s = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_UDP)
    s = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_ICMP)
    # following sniff all link layer frames, both incoming and outgoing
    s = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.ntohs(0x0003))
```

### references
- [blog: python packet sniffer](https://www.binarytides.com/python-packet-sniffer-code-linux/)
