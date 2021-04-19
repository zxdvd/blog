```metadata
tags: c, basic
```

## c basic: type conversion

### type of numeric constant
For literal numeric constant like `100, 1000, 100000.1234, 200L, 100f`, you may wonder
 that what's the type of it? Is it int, unsigned int, long int or float?

The section 6.4.4.1 of C99 specification, integer constants, has rules for integer. And
 section 6.4.4.2 has rules for float constant.

Following is official rule for integer.

```
 The type of an integer constant is the first of the corresponding list in which its value
 can be represented.
```

You can check the detailed list in the referenced spec. A simple example, for a number
`100` or `1000000000`, it will first try with **int**. If **int** is enough, then it is
 **int**. Otherwise, it will try **long int**, **long long int** until the type is enough
 to hold the number.

If you define as `100U`. The candicate list is **unsigned int, unsigned long, unsigned long long**.
There is similar rules for suffix **l, lu, ll**.

For float constant, it is **double** if it doesn't have a suffix, **float** if suffix
 is **f** or **F**, **long double** if suffix is **l** or **L**.


### usual arithmetic conversions
It's very common to calculate multiple values of different types.

```c
float a = 1 + 2.5;          // int + double
int b = 1;
float c = 2.5;
double d = b + c + 100f;   // int + float + double
```

So will it do the calculation directly? Or it will do some conversion? If so, what's the
 rule of the conversion?

The section 6.3.1.8 of the spec defines rules about this. To sum up

  - If either side is float type (float or double), then the other side is also converted
    to float type in the order of **long double, double, float**. For example, if a float
    is added by double, then the float is converted to double.

  - For integer, the rules are checked from top to bottom (stop after matched):
      - integer promotion at first: type ranks lower than int are promoted to int (or unsigned int if int is not enough)
      - if both are signed or unsigned, then a rank upgrade made them to be same type
      - if rank of unsigned is greater or equal to rank of the signed, then the signed will be convert to unsigned
      - then rank of unsigned is less than rank of the signed, then the signed can hold all values of unsigned, then
        the unsigned is convert to signed

These rules works for following operators:

    - arithmetic operators: *, /, %, +, -
    - comparing operators: ==, !=, >, <, >=, <=
    - bitwise operators: &, |, ^

Above rules mentioned the concept of `rank`. The `rank` of integer types from low to high are:

    bool  <  char  <  short  <  int  <  long  <  long long

### arithmetic calculation and conversion
How does C deal with integer overflow? For example, a small unsigned int substracts from a
 large one and got a negative integer, or two integer multiples and got a very large integer.

#### unsigned int overflow
Actually, operations on unsigned int never overflow. The result will be modulo to fit in
 proper range. For example, `(uint8)1 - (uint8)2 = -1 % 2^8 = 255`,
 `(uint8)10 * (uint8)30 = 300 % 2^8 = 44`.

#### int overflow
However, `int overflow` will get undefined behavior and you must avoid it. You can convert
 it to a large enough type like int32, int64 or float.

### references
- [c spec: ISO/IEC 9899](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1124.pdf)
- [o'reilly: C in a nutshell](https://www.oreilly.com/library/view/c-in-a/0596006977/ch04.html)
- [wikipedia: integer overflow](https://en.wikipedia.org/wiki/Integer_overflow)
