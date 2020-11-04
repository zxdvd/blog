```metadata
tags: algorithm, crypto, key-exchange, DH, ssl
```

## crypto: Diffie Hellman key exchange algorithm

The Diffie Hellman key exchange algorithm is a method to exchange shared secret key
 over a public network. Public network means that man in the middle can capture all
 the messages you sent.

I used to get a blog that explains this using color and it's really easy to understand.
 I read it long time ago and I don't remeber the origin.

Suppose A and B need to communicate with each other over a public network. They have
 a shared color `color1` and everybody knows it.

And each of them has a private color that nobody knows. Suppose A has color2 and B has
 color3.

Then each of them mixes the shared color1 with their private color and sends to another
 side.

Finally, they both mix the received mixed color with private color. And both of them
 get `color1+color2+color3`.

Suppose C is man in the middle, and we can get following table:

|A------------------|C-----------------|B---------------------|
|-------------------|------------------|----------------------|
|color1             | color1           |  color1              |
|color2             |                  |  color3              |
|color1color2       | color1color2     |  color1color3        |
|color1color3       | color1color3     |  color1color2        |
|color1color3color2 |                  |  color1color2color3  |

C knows color1, and he can capture `color1+color2` and `color1+color3`. However, he can
 not get `color1+color2+color3` since he can not split each of `color2` and `color3`
 from the mixed color. Then A and B finish the key exchange safely.

Actually, DH uses modular exponentiation internally.

- Both A and B agree on public number g (base) and p (large prime), it's like color1 above.

- A has a private number `a`, and it calculates `g^a mod p` and sends it to B. While B
 has private `b` and B sends `g^b mod p` to A.

- A gets `g^b mod p`, and it can calculate `(g^b mod p)^a mod p`. While B can calculate
 `(g^a mod p)^b mod p`.

Actually, `(g^b mod p)^a mod p` = `(g^b^a) mod p` = `(g^a^b) mod p` = `(g^a mod p)^b mod p`.
 You can get detail about this in article [fast modular exponentiation](./math: fast modular exponentiation.md).

Then A and B get a shared private key. And man in the middle can only capture `g^a mod p`
 and `g^b mod p` but he can not calculate `a` and `b` from them (it takes very long time
 to brute force) so he can not get the shared key.

### references
- [math: diffie-hellman](http://www.math-cs.gordon.edu/courses/mat231/notes/diffie-hellman.pdf)
