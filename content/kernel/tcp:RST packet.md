<!---
tags: linux, network, tcp
-->

## The RST packet


### RST processing
A RST should be validated first, invalid RST is ignored. In all states except
 SYN-SENT, the sequence number should be in the receive window. And in the
 SYN-SENT state, the RST is invalid if the ACK field acknowledges the SYN (
 since it's the first packet from another side, any sequence number is OK).

If the receiver is in SYN-RECEIVED and came from LISTEN, then it will go to
 LISTEN if got valid RST. For other state, the receiver will abort the connection
 and go to CLOSED state.


### why and when got RST
- connect to a port that has no server listen on it
- firewall may rejec the connection via RST (you can config iptables to reject)
- routers in the middle may send RST
- userspace close socket without read received buffer in the kernel
- send to a closed socket

### how to send a RST
You can use the [SO_LINER option](./tcp:%20socket%20options.md) to send a RST on close
 instead of FIN. Following are demo of python and c:

```python
s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii', 1, 0))
s.close()
```

```c
    struct linger lg = { .l_onoff = 1, .l_linger= 0};
    if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg)) != 0) {
        // failed to set SO_LINGER
    }
    close(sockfd);
```

### references
- [rfc 793 TCP](https://tools.ietf.org/html/rfc793)
