```metadata
tags: go, basic, map
```

## go basic: map
Tips about map.

### nil map
`nil map` is not empty map. And some behaviors are really different from other languages.

- You can iterate nil map normally. You won't get error, it just do nothing.
- You can access key of nil map without panic.
- You CANNOT set key value of a nil map, you need to initiate it.

```go
	var m1 map[string]bool = nil
    for k,v := range m1 {                     // it's ok to loop nil map
		fmt.Println("k,v", k, v)
	}
	fmt.Println("access key of nil map", m1["hello"])         // won't panic

    m1["hello"] = true              // this will panic(no error when building)
```

### float key NaN
`math.NaN` (not-a-number) is defined as a special float number. It has a very special
 property: `NaN != NaN`. Then you need to be careful when using it as key of map.

For javascript, NaN is considered the same even though `NaN !== NaN`, so you'll only
 have one NaN key in `Map` of javascript.

For python and go, you can have many NaN keys at the same time.

```go
	a := math.NaN()
	m1 := map[float64]bool{}
	for i := 0; i < 5; i++ {
		m1[a] = true
	}
	for i, v := range m1 {
		fmt.Println("KV pair in map", i, v)
	}
	_, ok := m1[a]
	fmt.Println("is key NaN exists: ", ok)

// output
KV pair in map NaN true
KV pair in map NaN true
KV pair in map NaN true
KV pair in map NaN true
KV pair in map NaN true
is key NaN exists:  false
```

From above code, we can find that the same `NaN` was insert as new key multiple times.
 We can iterate to list them all but we cannot get any of them via the same NaN key.

So when we use float as key of map, we need to think about whether it can be NaN or not
 especially when the key is calculated on the fly. We need to take care about this.
 Otherwise, it may fill the map to burst.

### key type restriction
For key of map, most languages require `equal` method and `hash` method. According to
 [go spec](https://golang.org/ref/spec#Map_types). Key type needs to define comparision
 operators == and !=. Thus function, map and slice cannot be key of map.

So can a struct be key of map?

That depends. According to go spec:

    Struct values are comparable if all their fields are comparable.

So if all fields of struct is comparable, then it can be key of map. If any of them
 is slice or map, then it cannot be key of map.

Same to array, an array can be key of map if all elements are comparable.

### iterate order
I used to write a blog about [hash order](../data_structure/hash table and order.md).
 So how about iterate order of go map?

Unlike to python (from version 3.7) and js which iterate using inserted order, you
 should never depend on iterating order of go map. And go adds randomness for starting
 of an iteration to enforce the unorder so that two iteration may have totally different
 order even you didn't change the map at all.

### map only grows never shrink
Currently, map only grows and never shrinks. If you add many many elements to a map
 and then remove most of them, memory of key value pairs will be garbage collected
 but internal buckets of map still kept.

If you want the map to be shrinked, you need to do it by yourself, like copy to a new
 map.

### map grows gradually
According to article about map growing, we know that map grows gradually and it moves
 a little when you set or delete keys from the map.

So for the case that loading large amount of data and using it as a read-only map,
 you'd better initiate it with good hint to avoid leaving it in the middle of a
 growing. Otherwise, you are wasting memory of an extra bucket array.

### concurrent map
To use map in concurrent situations, you can use either a normal map that protected
 by `sync.Mutex` or `sync.Map`. So what's the difference?

Most of time, it is suggested to use map protected by `sync.Mutex`. According to the
 official document of `sync.Map`, `sync.Map` is suggested in following cases:

    // The Map type is optimized for two common use cases: (1) when the entry for a given
    // key is only ever written once but read many times, as in caches that only grow,
    // or (2) when multiple goroutines read, write, and overwrite entries for disjoint
    // sets of keys. In these two cases, use of a Map may significantly reduce lock
    // contention compared to a Go map paired with a separate Mutex or RWMutex.

The `sync.Map` has a shallow map that stored using `atomic.Value`. For cases that
 reads for most time, it may not use the internal `Mutex`.

```go
// sync/map.go
type Map struct {
	mu Mutex
	read atomic.Value // readOnly
	dirty map[interface{}]*entry
}

// readOnly is an immutable struct stored atomically in the Map.read field.
type readOnly struct {
	m       map[interface{}]*entry
	amended bool // true if the dirty map contains some key not in m.
}

func (m *Map) Load(key interface{}) (value interface{}, ok bool) {
	read, _ := m.read.Load().(readOnly)
	e, ok := read.m[key]
	if !ok && read.amended {
		m.mu.Lock()
        // write dirty map
		m.mu.Unlock()
	}
	if !ok {
		return nil, false
	}
	return e.load()
}
```

Another bad side of `sync.Map` is that it uses `interface{}` instead of specific
 type. It's not safe.

### references
- [go spec: compare operators](https://golang.org/ref/spec#Comparison_operators)
- [go blog: map](https://blog.golang.org/maps)
- [go source analyse: map growing](./sourcecode/runtime: map part3 - growing.md)
