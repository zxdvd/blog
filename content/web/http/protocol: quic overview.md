```metadata
tags: web, http, quic, udp
```

## quic overview

`QUIC` is initially designed at google and aims to be successor of HTTP/2. It provides
 reliable, ordered streams on top of unreliable, unordered UDP protocol.

### why UDP
UDP is connectionless so that the three-way handshake is avoided. And QUIC uses TLS 1.3
 so that only 1-RTT first page latency. This improves loading time of new site dramatically.

UDP has small header and it gives chance to improve throughput a little.

UDP has no congestion control and flow control. I don't mean that this is advantage.
 But it do give opportunity to high level protocols like QUIC to implement their own
 congestion control and flow control.

Another important reason is the `head byte block` of TCP. We've heard of head byte block
 of HTTP 1.1. However, this also affects HTTP/2. If one packet is lost or delayed in
 TCP, all later packets are blocked since TCP gurantees reliability and ordering. Then
 even though HTTP/2 supports multiplex, all streams may be blocked if one packet got lost.

With UDP, one packet losing or delaying only affects streams packed in this packet. It
 doesn't block all streams.

### multiplex
QUIC learns the multiplex feature of HTTP/2. A QUIC connection can carry many logical
 ordered streams, each stream is like an ordinary TCP connection. Both client and server
 can open a new stream. To avoid collision, server side uses even number from 2 as
 stream id while client side uses odd number. 0 is invalid and 1 is used for crypto
 handshake.

### adaptable to network migration
We know that a TCP connection is identified by 4 tuples of source ip, source port, destination
 ip and destination port. It will fail if any one of the 4 changed and then you need to
 reconect.

QUIC doesn't depend on any of them. It uses connectionId as identifier so that it will
 continue smothly while network migration.

### 0-RTT
We often see QUIC together with 0-RTT (round trip time). Is QUIC really 0-RTT all the time?

Actually, QUIC 0-RTT only works for resumption while 0-RTT was enabled. There is no
 connection handshake overhead since UDP but not TCP is used. And due to TLS 1.3, TLS
 handshake RTT is reduced to 1-RTT for new connection. So 1-RTT for new connection and
 0-RTT for resumption if enabled (for security reason, 0-RTT may be disabled, see
 [here](./../../algorithm/crypto: TLS 1.3 and 0-RTT.md)).


### references
1. [chrome doc: quic](https://www.chromium.org/quic)
2. [ietf: quic draft spec](https://tools.ietf.org/id/draft-ietf-quic-transport-32.html)
3. [cloudflare: http3 past and future](https://blog.cloudflare.com/http3-the-past-present-and-future/)
