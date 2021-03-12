```metadata
tags: tls, openssl, ca
```

## tls openssl tools

### RSA key
You can generage/show RSA key using `openssl genrsa` command.

    $ openssl genrsa -out rsa.key 2048

Show parameters of a RSA key:

```
$ openssl rsa -in ca.key -text -noout

RSA Private-Key: (2048 bit, 2 primes)
modulus:
    00:ab:59:3c:10:76:ed:aa:77:e2:c1:b3:b7:4a:9f:
publicExponent: 65537 (0x10001)
privateExponent:
    4b:18:36:9c:b8:a2:7c:5d:42:41:c6:33:84:3e:b2:
prime1:
    00:c4:dc:eb:91:dd:28:9a:b5:26:2f:b6:91:11:ee:
prime2:
    00:de:d2:37:57:79:f9:f6:ac:7c:93:ed:2d:f6:9b:
exponent1:
    1e:28:9a:0e:26:8b:69:e8:06:89:98:b6:70:c0:cf:
...
```

The `modulus` and `publicExponent` of the private key composed the public key. `publicExponent`
 is usually 65537.

#### CSR (certificate signing request)
Generate a CSR using private key. You can send this to CA and request CA to sign it
 to get a signed certificate.

    $ openssl req -new -key rsa.key -out cert.csr

To show content of CSR, using following command:

    $ openssl req -in cert.csr -text -noout

You can also create a CSR from a config file. By this way, you can add SAN (subjectAlternativeNames).

Create a file a.com.csr.confg like following:

```
[req]
distinguished_name = dn
req_extensions     = req_ext
prompt = no
[dn]
CN = "a.com"
[req_ext]
subjectAltName = @names
[names]
DNS.1 = a.a.com
DNS.2 = b.a.com
DNS.3 = c.a.com
```

And then generate CSR using:

    $ openssl req -new -key rsa.key -out cert.csr -config a.com.csr.config

### CA
To setup a CA, you can generate a key first:

    $ openssl genrsa -out ca.key 2048

And then generate a X509 certificate of the key. Following command will execute
 directly. It won't ask for information about country, organization and common name
 since you have specified common name via `-subj '/CN=example.com'`.

    $ openssl req -x509 -new -key ca.key -subj '/CN=example.com' -days 10000 -out ca.crt

#### CA sign a CSR
After setup the CA, now you can sign the CSR using the generated CA key and certificate.

    $ openssl x509 -req -in cert.csr -CA ca.crt -CAkey ca.key -CAcreateserial -days 365 -out cert.crt

CA will add serial number (unique per CA) issuer info, validity (not before, not after),
 signature algorithm and signature to the CSR to make a signed certificate.

#### sign a CSR with subjectAlternativeNames
You can only have one common name for each certificate. You can add alternative names
 via extension.

Create an extension file like `a.com.ext`:

```
subjectAltName = @names

[names]
DNS.1 = a.a.com
DNS.2 = b.a.com
DNS.3 = c.a.com
```

Then using the extension file via the `-extfile` option:

    $ openssl x509 -req -in cert.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
            -days 365 -out cert.crt -extfile a.com.ext

### certificate
Show content of a certificate:

```
$ openssl x509 -in ca.crt -text -noout

Certificate:
    Data:
        Version: 3 (0x2)
        Serial Number: 0 (0x0)
        Signature Algorithm: sha256WithRSAEncryption
        Issuer: CN = kubernetes
        Validity
            Not Before: Feb  5 06:54:51 2021 GMT
            Not After : Feb  3 06:54:51 2031 GMT
        Subject: CN = kubernetes
        Subject Public Key Info:
            Public Key Algorithm: rsaEncryption
                RSA Public-Key: (2048 bit)
                Modulus:
                    00:ab:59:3c:10:76:ed:aa:77:e2:c1:b3:b7:4a:9f: ...
                Exponent: 65537 (0x10001)
        X509v3 extensions:
            X509v3 Key Usage: critical
                Digital Signature, Key Encipherment, Certificate Sign
            X509v3 Basic Constraints: critical
                CA:TRUE
            X509v3 Subject Key Identifier:
                86:5B:54:3B:64:90:D8:6A:71:5F:EB:0D:FA:9F:2D:52:7F:3A:2A:06
    Signature Algorithm: sha256WithRSAEncryption
         22:37:6b:91:57:97:54:e6:1c:ee:f5:b6:54:8c:f4:51:a1:55: ...
```

### PEM format
PEM is a popular file format that used to store certificates and keys. A PEM file may
 have a suffix like `.pem`, `.cer`, `.crt` or `.key`. Following is part of a PEM
 certificate:

```
-----BEGIN CERTIFICATE-----
MIIC5zCCAc+gAwIBAgIBADANBgkqhkiG9w0BAQsFADAVMRMwEQYDVQQDEwprdWJl
cm5ldGVzMB4XDTIxMDIwNTA2NTQ1MVoXDTMxMDIwMzA2NTQ1MVowFTETMBEGA1UE
...
-----END CERTIFICATE-----
```

It begins with a `-----BEGIN <LABEL>-----` and ends with `-----END <LABEL>-----`.
Content in the middle is binary data encoded using base64. The label could be
 `CERTIFICATE`, `CERTIFICATE REQUEST`, `PKCS7`, `PRIVATE KEY`, `PUBLIC KEY` or others.

### other tools
- [mkcert](https://github.com/FiloSottile/mkcert) is a nice and easy to use tool that can
 help you to setup CA and create certificates in one line.

### references
- [wikipedia: PEM](https://en.wikipedia.org/wiki/Privacy-Enhanced_Mail)
- [gist: how to setup your own CA](https://gist.github.com/Soarez/9688998)
