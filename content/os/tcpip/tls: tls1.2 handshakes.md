```metadata
tags: tcpip, crypto, ssl, tls
```

## TLS 1.2 handshakes

The handshakes of TLS 1.2 are summed up like:

    - negotiate version, cipher and parameters
    - exchange and authenticate certificate and exchange keys
    - agree on a shared key and begin to communicate based on this key

The detailed steps look like following (key exchanged with ECDH):

```
            client                                            server
---------------------------------------------------------------------------------------
1.    protocol version
      client random data            ---------->>>>
      cipher suite list
      extensions (Server Name, Sig Algo.)
---------------------------------------------------------------------------------------
2.                                                     selected protocol version
                                                       server random data
                                    <<<<---------      selected cipher suite
                                                       extensions
---------------------------------------------------------------------------------------
3.                                  <<<<---------      server certificate
---------------------------------------------------------------------------------------
4.                                                     server generates a pair public/private keys
                                                       server sign the public key with private key of certificate
                                    <<<<---------      server sends public key and sign to client
                                                       server hello done
---------------------------------------------------------------------------------------
5.    client generates a pair public/private keys
      client sends public key to server                  -------->>>>
      client generates premaster key via client private
            key and server public key
      client generates master key via premaster key
            and client random and server random
      client change cipher spec                          -------->>>>
      client handshake finished                          -------->>>>
---------------------------------------------------------------------------------------
6.                                  <<<<---------      server change cipher spec
                                    <<<<---------      server handshake finished
---------------------------------------------------------------------------------------
7.    application data            ---->>>
                                  <<<----              application data
                                ..........
---------------------------------------------------------------------------------------
```

### cipher cuites
TLS 1.2 supports nearly 60 cipher suites (`openssl ciphers -tls1_2 -v`). A cipher suite
 contains 4 algorithm: key exchange algorithm, authentication algorithm, encryption
 algorithm and MAC algorithm. Take `ECDHE-RSA-AES128-SHA256` as an example, `ECDHE` is
 used for key exchange, `RSA` for authentication of the certificate, `AES128` for data
 encryption and `SHA256` for message authentication.

### key exchange: RSA and ECDH
Above handshake uses `ECDH` for key exchange, thus each sides generate a pair of private
 and public keys. Then they can negotiate a shared premaster key.

For `RSA` key exchange algorithm, it's a little different. The client generates a premaster
 key, and it encrypts the key using public key on the server's certificate and sends it
 to the server. The server can decrypt it using private key of the certificate.

The bad side of `RSA` is that it doesn't support `forward secrecy`. So what is FS or PFS
 (perfect forward secrecy)?

Man in the middle can capture all the traffics, but it cannot decrypt them without private
 key of the certificate. But the private key may be leaked or staled in the future. Then
 attackers can use it to decrypt saved traffics. They may get confidential information
 from these traffics.

With `ECDH` key changing algorithm, you don't have this problem. The private key of certificate
 is only used to sign the message to prove that it owns the certificate. Each side generate
 private keys in the memory and it is destroyed after they negotiated the master key.

So anyone saved the traffics cannot decrypt them even the private key of the certificate
 is leaked. So you can achieve PFS via `ECDH`.

### key calculation
The master key is used as seed to generate MAC key, write key and IV of both client and
 server. It uses PRF function to create enough bytes.

Length of MAC key, write key and IV varies between different cipher cuites. For example,
 the six parts of `AES_128_CBC_SHA256` are:

    32 bytes MAC key for client
    32 bytes MAC key for server
    16 bytes write key for client
    16 bytes write key for server
    16 bytes IV for client
    16 bytes IV for server

### optional client certificate
The server can also send a requesting message to ask the client to provide certificate.
 This may be used in internal cluster communication. For normal https communication, it
 doesn't request client certificate.

### change cipher spec
Both client and server will send the `change cipher spec` message. This message indicates
 end of handshakes. And all following messages will be encrypted using keys.

### FAQ

#### why premaster secret to master secret?
You can easily calculate master secret from premaster secret. So why this extra calculation?
TLS can use a lot of cipher cuites. Different cipher cuite may get different size of premaster
 secret. Then TLS provides a uniform function to convert premaster secret of any size to a
 fixed 48 bytes size.

    master_secret = PRF(pre_master_secret, "master secret",
                          ClientHello.random + ServerHello.random)
                          [0..47];

#### how did the client verify that the server owns the certificate?
Client can verify that whether the certificate is signed by trusted CA or not. But how did
 it verify that the server owns the certificate? Man in the middle can also use same certificate
 of target site.

In step 4, the server generates a pair of private/public keys. And it signs the public key
 with `private key of the certificate`. Then client can verify the signature using public
 key in the certificate. The others (man in the middle) which don't have the private key
 of the certificate cannot sign the signature.

By this way, client can verify the public key is issued by certificate owner.


### references
- [illustrated TSL connection 1.2](https://tls.ulfheim.net/)
- [rfc5246: computing the master secret](https://tools.ietf.org/html/rfc5246#page-64)
- [rfc5246: key calculation](https://tools.ietf.org/html/rfc5246#page-26)
