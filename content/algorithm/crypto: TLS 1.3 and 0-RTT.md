```metadata
tags: algorithm, crypto, ssl, tls
```

## crypto: TLS 1.3

TLS 1.3 is the newest version of TLS, it improves security and latency. It's supported
 in Chrome from version 66.

TLS 1.2 is the most popular right now. It supports many cipher suites. However, some cipher
 suites are obsolete and not safe, like RC4, 3DES.

And TLS 1.2 needs 2 extra round trips besides TCP handshake. This really hurts first page
 load time a lot. The `hello` message and `key exchange` are separated since `hello`
 contains cipher negotiation and `key exchange` depends on the selected cipher.

However, in TLS 1.3, the `key exchange` algorithm was specified to DH only (RSA removed),
 so that the `hello` and `key exchange` info are mreged to a same message. And only one
 round trip is enough.

For security side, TLS 1.3 removed a lot of obsolete cipher suites which is applicable
 in TLS 1.2 to improve security.

Supported ciphers in TLS 1.3

    TLS_AES_256_GCM_SHA384
    TLS_CHACHA20_POLY1305_SHA256
    TLS_AES_128_GCM_SHA256

### 0-RTT resumption
TLS 1.3 supports 0-RTT resumption with some caveats while TLS 1.2 is 1-RTT. When
 reconnecting to a TLS 1.3 server, you can send early data which is encrypted via the
 PSK (pre-shared key) along with the client hello message. The PSK is generated at
 previous connection. All the following data are encrypted with the exchanged key but
 not the PSK.

The 0-RTT leads to following problems so that it may not be an option and you can disable it.

- It's vulnerable to replay attack. If the early data is a POST form like transfer
 money, man in the middle may capture the hello message and replay it.

- No Forward Secrecy. Server needs key to unwrap PSK from session ticket, if the key
 is lost later in the future, attack can use it to decrypt early data from captured
 packets.

For general application, like blog sites or CDN for js or videos, early data is a simple
 HTTP GET request, 0-RTT is more valuable compared to these small caveats.

### references
1. [OWASP: overview of TLS 1.3](https://owasp.org/www-chapter-london/assets/slides/OWASPLondon20180125_TLSv1.3_Andy_Brodie.pdf)
2. [cloudflare: TLS 1.3 overview and q&a](https://blog.cloudflare.com/tls-1-3-overview-and-q-and-a/)
3. [blog: 2019 - TLS 1.3 Everything you need to know](https://www.thesslstore.com/blog/tls-1-3-everything-possibly-needed-know/)
4. [TLS 1.3 packet explained and reproduced](https://tls13.ulfheim.net/)
