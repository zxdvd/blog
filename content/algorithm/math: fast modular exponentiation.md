```metadata
tags: algorithm, math, modular
```

## math: fast modular exponentiation

A method to compute `A^B mod C` quickly, which is commonly used in computer science
 especially crypto area.

### modular multiplication
Let's prove `A*B mod C = (A mod C * B mod C) mod C` first.

    Suppose A = k1*C + b1, B = k2*C + b2
    A*B mod C = (k1*C + b1)*(k2*C + b2) mod C
              = (k1*k2*C*C + b1*k2*C + b2*k1*C + b1*b2) mod C
              = b1*b2 mod C
              = (A mod C)*(B mod C) mod C    // b1 = A mod C since A=k1*C+b1

And we can get `A^2 mod C == (A mod C)^2 mod C`, and `A^B mod C = (A mod C)^B mod C`.

So `83^213 mod 53` = `(83 mod 53)^213 mod 53` = `30^213 mod 53`.

### simple method
Then is there any way to compute it fast when B is very large?

Take B = 213 as an example

    213 = 2^7 + 2^6 + 2^4 + 2^2 + 2^0
    30^213 = 30^(2^7 + 2^6 + 2^4 + 2^2 + 2^0)
           = 30^128 * 30^64 * 30^16 * 30^4 * 30^1
    30^213 mod 53 = (30^128 * 30^64 * 30^16 * 30^4 * 30^1) mod 53
       = ((30^128) mod 53 * (30^64) mod 53 * (30^16) mod 53 * (30^4) mod 53 * 30 mod 53) mod 53

And we can calculate them from small to large since they have dependencies.

    30 mod 53 = 30
    30^2 mod 53 = 52
    30^4 mod 53 = (30^2 mod 53 * 30^2 mod 53) mod 53 = (52 * 52) mod 53 = 1
    30^8 mod 53 = (30^4 mod 53 * 30^4 mod 53) mod 53 = (1 * 1) mod 53 = 1
    30^16 mod 53 = (30^8 mod 53 * 30^8 mod 53) mod 53 = (1 * 1) mod 53 = 1
    30^32 mod 53 = (30^16 mod 53 * 30^16 mod 53) mod 53 = (1 * 1) mod 53 = 1
    30^64 mod 53 = (1 * 1) mod 53 = 1
    30^128 mod 53 = (1 * 1) mod 53 = 1

Then

    30^213 mod 53 = ((30^128) mod 53 * (30^64) mod 53 * (30^16) mod 53 * (30^4) mod 53 * 30 mod 53) mod 53
                  = (30 * 1 * 1 * 1 * 1) mod 53
                  = 30

A simple python function to calculate the modular:

```python
def fme(base,exp,mod):
    m = base % mod
    mul = 1
    while exp:              # loop over exp from lowest bit to highest bit
        if exp % 2 == 1:
            mul = mul * m % mod
        m = (m * m) % mod
        exp = exp >> 1
    return mul % mod
```

### calculate fme using Euler's Theorem
The Euler's Theorem is `a^φ(n) mod n = 1` while n is a positive integer and a and n are coprime.
`φ(n)` or `phi(n)` is the totient function. It is defined to be the number of positive numbers
 less than n and coprime to n.

    For n=p1^e1 * p2^e2 * p3^e3 ... pi^ei,
    φ(n) = n * (1-1/p1) * (1-1/p2) * ...(1-1/pi)
    If n is prime number, φ(n) = n-1.

Then

    Suppose B = k * φ(C) + n
    A^B mod C = A^(k * φ(C) + n) mod C
              = A^(φ(C) * k) * A^n mod C
              = (A^φ(C)) ^ k) * A^n mod C
              = (A^φ(C) mod C) ^ k) * A^n mod C   // modular multiplication
              = (1^k) * A^n mod C                 // A^φ(C) mod C=1 if A and C are coprime
              = A^n mod C

A example from the comment in Ref. 2:

    R = 5^123456789 mod 52
    since φ(52) = 52 * (1-1/2) * (1-1/13) = 24
    then  5^φ(52) mod 52 = 5^24 mod 52 = 1         // Euler's Theorem
    then R = 5^123456789 mod 52
           = 5^(51440 * 24 + 21) mod 52
           = 5^21 mod 52                          // the exp scale down to very small


### references
1. [khan: fast modular exponentiation](https://www.khanacademy.org/computing/computer-science/cryptography/modarithmetic/a/fast-modular-exponentiation)
2. [Euler's Totient function and Euler's Theorem](https://www.doc.ic.ac.uk/~mrh/330tutor/ch05s02.html)
