```metadata
tags: basic, float
```

## float number

We can store and calculate integers accurately but we can't with float since integer
 is discrete while float is continuous. It leads to many problems about precision.

### special values
Float has some special values defined, like `NaN`, `-NaN`, `Infinity`, `-Infinity`.

`NaN` has a very special property: `NaN != NaN`. The `math.IsNaN` of go is defined
 like following:

```go
// go: src/math/bits.go
func IsNaN(f float64) (is bool) {
	return f != f
}
```

From `math.NaN()` and `math.Inf()`, we can know how these variables defined.

```go
// go: src/math/bits.go
const (
	uvnan    = 0x7FF8000000000001
	uvinf    = 0x7FF0000000000000
	uvneginf = 0xFFF0000000000000
)

func Inf(sign int) float64 {
	var v uint64
	if sign >= 0 {
		v = uvinf
	} else {
		v = uvneginf
	}
	return Float64frombits(v)
}

// NaN returns an IEEE 754 ``not-a-number'' value.
func NaN() float64 { return Float64frombits(uvnan) }
```

#### serialization of special values
It seems that IEEE 754 defined NaN very relaxed and it left a very large range for NaNs. So that
 different architectures, compilers may have different representation of NaNs. It may lead
 to problem for float/double serialization and deserialization.

We can learn how to deal with it from redis,
[commit a7866d](https://github.com/redis/redis/commit/a7866db6cc5f68cd577bc9684d10bb048d63788f) is about this.

```c
static double R_Zero, R_PosInf, R_NegInf, R_Nan;

static void initServerConfig() {
    /* Double constants initialization */
    R_Zero = 0.0;
    R_PosInf = 1.0/R_Zero;
    R_NegInf = -1.0/R_Zero;
    R_Nan = R_Zero/R_Zero;
}

static int rdbSaveDoubleValue(FILE *fp, double val) {
    unsigned char buf[128];
    int len;

    if (isnan(val)) {
        buf[0] = 253;
        len = 1;
    } else if (!isfinite(val)) {
        len = 1;
        buf[0] = (val < 0) ? 255 : 254;
    }
    ...
}
```

It initializes `inf`, `-inf`, `nan` at runtime but not compile time to avoid compiler
 optimization about these values.

It doesn't actually serialize these values. It encodes them in metadata (use buffer
 length 253,254,255 to represent the three).

### float spec (IEEE 754)
A float is composed by 3 parts: sign bit, exponent bits and fraction bits. And there is
 single precision float (32bit) and double precision float (double, 64 bit).

![float and double](./images/float.png)

A float is represented as `sign * fraction * 2^exp`.

Take 32 bit float as example, the sign bit indicates whether it's positive (0) or negative (1).
The 8 bits exponent means `exp` in above formula. The range of 8 bits exponent is 0-255.
The actual `exp` is `exp - 127` so that it can represent both small values and large values.

The fraction part is something like `1.fraction` and the left 23 bits are used for fraction
 after the dot, the `1.` is added implicitly.

So the actual formula is like following:

    (-1)^sign_bit  X  2^(exp_value - 127)  X  (1 + fraction_value)

### decimal calculation
Since we maynot represent float accurately, then you need to be very careful when calculation
 with float. See following example (node REPL):

```js
> a = 0.1+0.2
0.30000000000000004
> a === 0.3
false
```

To avoid these precision problems, we can define an acceptable delta and compare like following:

```js
    const delta = 1e-8
    function equal(a,b) {
        return Math.abs(a-b) < delta
    }
```

We can also use decimal module to calculate using decimal instead of binary. Some languages
 like python have builtin decimal module, some don't so that you need to find a third-part
 library.

I read source code of python's decimal module a little, it will try to convert float to
 integer if necessary since integer calculation has no precision loss.

### references
- [wikipedia: float](https://en.wikipedia.org/wiki/Single-precision_floating-point_format)
- [IEEE 754](https://irem.univ-reunion.fr/IMG/pdf/ieee-754-2008.pdf)
- [python source code: decimal](https://github.com/python/cpython/blob/3.7/Lib/_pydecimal.py)
