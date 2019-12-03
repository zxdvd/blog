<!---
tags: go, performance
-->

Recently, I wrote a simple [datetime format library](https://github.com/zxdvd/go-libs/tree/master/datetime)
that uses the moment.js style. I think it's necessary to know the performance compared with 
the standard library. Then I added two simple benchmark.

```go
func BenchmarkFormat(b *testing.B) {
	t := time.Date(2009, time.November, 10, 23, 5, 16, 45000000, time.UTC)
	for i := 0; i < b.N; i++ {
		_ = Format(t, "YYYY-MM-DD HH:mm:ss.SSS")
	}
}

func BenchmarkStdFormat(b *testing.B) {
	t := time.Date(2009, time.November, 10, 23, 5, 16, 45000000, time.UTC)
	for i := 0; i < b.N; i++ {
		_ = t.Format("2006-01-02 15:04:05.000")
	}
}
```

Then I got the benchmark result like following
```shell
$ go test -bench=. -benchmem -run ^$
BenchmarkFormat-4        2000000              1037 ns/op             128 B/op          7 allocs/op
BenchmarkStdFormat-4     3000000               479 ns/op              32 B/op          1 allocs/op
```

The standard library was 1.5 times faster than mine and used only 1 allocation while mine used 7.

Then I checked my code and tried to find all the allocations and eliminate them.

### use array buffer instead of make
I changed the `b = make([]byte, 0, buflen)` to following. It utilized a array as buffer for small size.
Then the benchmark shows that it reduces one allocation.

```go
	var b []byte
	var layoutBuffer [64]byte
	if buflen < 64 {
		b = layoutBuffer[:0]
	} else {
		b = make([]byte, 0, buflen)
	}
```

Attention, use array as buffer do not promise to allocate on stack. If the escape analysis thinks that 
it should be allocate on heap then it will.

### use strconv.AppendInt instead of strconv.Itoa
The strconv.Itoa returns a string. Then it will do allocation internally of course. While the 
strconv.AppendInt will reuse the bytes buffer passed in. So always choose AppendInt to reduce allocation.

The benchmark shows that this reduces 1 allocation.

### drop bytes.Repeat
The `prepad` function will prepend a bytes with char ('1' to '001'). Below is the old code

```go
    buf := bytes.Repeat([]byte{p}, width-len(b))
    return append(buf, b...)
```

So much allocations, the `buf` is a new allocation and then the `append` may grow the `buf` again.

I optimized it by checking the capability of the buffer at fist to decide whether need to allocate 
a large buffer. Then do a `memmove` and padding.

The benchmark shows that this reduces 4 allocations and finally my code uses only 1 allocation just 
like the standard library. And the performance gap is reduced to 30% from 150%.

### is it possible to reduce to 0 allocation?
The only one allocation left is the `return string(b)` that convert the []byte to string. Is it 
possible to reduce it?

What about return []byte instead of string?

You may find `var layoutBuffer [64]byte` became a new allocation if you return []byte. The escape 
analysis at compiling time know that the []byte will escape then it won't be allocated at stack. 
It will be on heap.

So we cannot reduce this allocation until receiving a buffer via parameter. Thus zero allocation.

### reference
- [source code: golang time](https://github.com/golang/go/blob/master/src/time/format.go)
- [blog: high performance go workshop](https://dave.cheney.net/high-performance-go-workshop/dotgo-paris.html#benchmarking)
