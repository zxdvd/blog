```metadata
tags: go, sourecode, runtime, map, hashmap
```

## go runtime: map part2
The part1 of go map shows a overview picture. The part 2 will go through the process
 of create, get and set of map.


### map create
`map` can be created based on an old map and a hint is given to help to initiate
 buckets. For small map, the it won't create buckets at initiation. The buckets
 will be created when adding key to it.

The `makeBucketArray()` function may also return an array of overflow buckets.

```go
func makemap(t *maptype, hint int, h *hmap) *hmap {
	if h == nil {
		h = new(hmap)
	}
	h.hash0 = fastrand()

    // skipped init h.B according to hint

	if h.B != 0 {
		var nextOverflow *bmap
		h.buckets, nextOverflow = makeBucketArray(t, h.B, nil)
		if nextOverflow != nil {
			h.extra = new(mapextra)
			h.extra.nextOverflow = nextOverflow
		}
	}

	return h
}
```

The `makeBucketArray()` will allocate about extra 1/16 buckets for overflow when h.B
 is large than 4.

```go
func makeBucketArray(t *maptype, b uint8, dirtyalloc unsafe.Pointer) (buckets unsafe.Pointer, nextOverflow *bmap) {
	base := bucketShift(b)
	nbuckets := base
	// For small b, overflow buckets are unlikely.
	// Avoid the overhead of the calculation.
	if b >= 4 {
		nbuckets += bucketShift(b - 4)
		sz := t.bucket.size * nbuckets
		up := roundupsize(sz)
		if up != sz {
			nbuckets = up / t.bucket.size
		}
	}

	if dirtyalloc == nil {
		buckets = newarray(t.bucket, int(nbuckets))
	} else {
    // skipped
	}

	if base != nbuckets {
		// We preallocated some overflow buckets.
		// To keep the overhead of tracking these overflow buckets to a minimum,
		// we use the convention that if a preallocated overflow bucket's overflow
		// pointer is nil, then there are more available by bumping the pointer.
		// We need a safe non-nil pointer for the last overflow bucket; just use buckets.
		nextOverflow = (*bmap)(add(buckets, base*uintptr(t.bucketsize)))
		last := (*bmap)(add(buckets, (nbuckets-1)*uintptr(t.bucketsize)))
		last.setoverflow(t, (*bmap)(buckets))
	}
	return buckets, nextOverflow
}
```

### map set
The `func mapassign(t *maptype, h *hmap, key unsafe.Pointer) unsafe.Pointer` function
 handles set operation of map. It looks a little strange that the parameters don't
 have value of the key. Actually, it returns pointer of the value and the caller
 need to update the pointer accordingly.

Go map is not concurrency safe. It'll set a flag during writing and check the flag
 at beginning of get and set and throw error when it indicates that another goroutine
 is writing it.

We know that go uses a gradually resize and now we can find that it tries to grow a
 little each time you set the map.

It gets the bucket via hash result. And loop all the 8 slots in this bucket and try
 to find the key.

If the key was found, it resets the writing flag and returns pointer to value.

If not find, it tries to mark a empty slot that may be used for inserting the key later,
 and it goes to next bucket, the overflow bucket. If not found, tries next overflow
 bucket until finds one or the overflow points to nil.

If it still doesn't get find the key, it tries to grow if it overflows too much and
 then tries from beginning.

If it didn't find the key and the empty slot to insert, then it will create an overflow
 bucket and use the first slot in it.

We can find that the set operations only deal with `h.buckets` and ignore the `h.oldbuckets`.
That's because it did a `growWork(t, h, bucket)` immediately after getting bucket number
 to evacuate affected bucket to avoid lookup of old bucket.

```go
// Like mapaccess, but allocates a slot for the key if it is not present in the map.
func mapassign(t *maptype, h *hmap, key unsafe.Pointer) unsafe.Pointer {
	if h == nil {
		panic(plainError("assignment to entry in nil map"))
	}
	if h.flags&hashWriting != 0 {
		throw("concurrent map writes")
	}
	alg := t.key.alg
	hash := alg.hash(key, uintptr(h.hash0))

	// Set hashWriting after calling alg.hash, since alg.hash may panic,
	// in which case we have not actually done a write.
	h.flags ^= hashWriting

	if h.buckets == nil {
		h.buckets = newobject(t.bucket) // newarray(t.bucket, 1)
	}

again:
	bucket := hash & bucketMask(h.B)
	if h.growing() {
		growWork(t, h, bucket)
	}
	b := (*bmap)(unsafe.Pointer(uintptr(h.buckets) + bucket*uintptr(t.bucketsize)))
	top := tophash(hash)

	var inserti *uint8
	var insertk unsafe.Pointer
	var elem unsafe.Pointer
bucketloop:
	for {
		for i := uintptr(0); i < bucketCnt; i++ {
			if b.tophash[i] != top {
				if isEmpty(b.tophash[i]) && inserti == nil {
					inserti = &b.tophash[i]
					insertk = add(unsafe.Pointer(b), dataOffset+i*uintptr(t.keysize))
					elem = add(unsafe.Pointer(b), dataOffset+bucketCnt*uintptr(t.keysize)+i*uintptr(t.elemsize))
				}
				if b.tophash[i] == emptyRest {
					break bucketloop
				}
				continue
			}
			k := add(unsafe.Pointer(b), dataOffset+i*uintptr(t.keysize))
			if t.indirectkey() {
				k = *((*unsafe.Pointer)(k))
			}
			if !alg.equal(key, k) {
				continue
			}
			// already have a mapping for key. Update it.
			if t.needkeyupdate() {
				typedmemmove(t.key, k, key)
			}
			elem = add(unsafe.Pointer(b), dataOffset+bucketCnt*uintptr(t.keysize)+i*uintptr(t.elemsize))
			goto done
		}
		ovf := b.overflow(t)
		if ovf == nil {
			break
		}
		b = ovf
	}

	// Did not find mapping for key. Allocate new cell & add entry.

	// If we hit the max load factor or we have too many overflow buckets,
	// and we're not already in the middle of growing, start growing.
	if !h.growing() && (overLoadFactor(h.count+1, h.B) || tooManyOverflowBuckets(h.noverflow, h.B)) {
		hashGrow(t, h)
		goto again // Growing the table invalidates everything, so try again
	}

	if inserti == nil {
		// all current buckets are full, allocate a new one.
		newb := h.newoverflow(t, b)
		inserti = &newb.tophash[0]
		insertk = add(unsafe.Pointer(newb), dataOffset)
		elem = add(insertk, bucketCnt*uintptr(t.keysize))
	}

	// store new key/elem at insert position
	if t.indirectkey() {
		kmem := newobject(t.key)
		*(*unsafe.Pointer)(insertk) = kmem
		insertk = kmem
	}
	if t.indirectelem() {
		vmem := newobject(t.elem)
		*(*unsafe.Pointer)(elem) = vmem
	}
	typedmemmove(t.key, insertk, key)
	*inserti = top
	h.count++

done:
	if h.flags&hashWriting == 0 {
		throw("concurrent map writes")
	}
	h.flags &^= hashWriting
	if t.indirectelem() {
		elem = *((*unsafe.Pointer)(elem))
	}
	return elem
}
```

### map get
There are 3 main functions about get operation. The `mapaccess1` and `mapaccess2` are
 almost same except that `mapaccess2` returns a second value to indicate whether the
 key exists or not. I don't understand why `mapaccess1` doesn't choose to call `mapaccess2`.

The get operation also check for concurrent writing at first. It's not safe to get
     while another is writing.

For get operation, it tries to find the bucket first. Considering about hash resizing,
 it may need to check `h.oldbuckets` too. It knows that which bucket to use since
 old bucket is flagged when it is evacuated.

The loop of chained bucket is similar to that in set operation. Just go through overflow
 bucket one by one.

    for ; b != nil; b = b.overflow(t) { }

```go
func mapaccess2(t *maptype, h *hmap, key unsafe.Pointer) (unsafe.Pointer, bool) {
	if h == nil || h.count == 0 {
		if t.hashMightPanic() {
			t.key.alg.hash(key, 0) // see issue 23734
		}
		return unsafe.Pointer(&zeroVal[0]), false
	}
	if h.flags&hashWriting != 0 {
		throw("concurrent map read and map write")
	}
	alg := t.key.alg
	hash := alg.hash(key, uintptr(h.hash0))
	m := bucketMask(h.B)
	b := (*bmap)(unsafe.Pointer(uintptr(h.buckets) + (hash&m)*uintptr(t.bucketsize)))
	if c := h.oldbuckets; c != nil {
		if !h.sameSizeGrow() {
			// There used to be half as many buckets; mask down one more power of two.
			m >>= 1
		}
		oldb := (*bmap)(unsafe.Pointer(uintptr(c) + (hash&m)*uintptr(t.bucketsize)))
		if !evacuated(oldb) {
			b = oldb
		}
	}
	top := tophash(hash)
bucketloop:
	for ; b != nil; b = b.overflow(t) {
		for i := uintptr(0); i < bucketCnt; i++ {
			if b.tophash[i] != top {
				if b.tophash[i] == emptyRest {
					break bucketloop
				}
				continue
			}
			k := add(unsafe.Pointer(b), dataOffset+i*uintptr(t.keysize))
			if t.indirectkey() {
				k = *((*unsafe.Pointer)(k))
			}
			if alg.equal(key, k) {
				e := add(unsafe.Pointer(b), dataOffset+bucketCnt*uintptr(t.keysize)+i*uintptr(t.elemsize))
				if t.indirectelem() {
					e = *((*unsafe.Pointer)(e))
				}
				return e, true
			}
		}
	}
	return unsafe.Pointer(&zeroVal[0]), false
}
```

### map delete
The `mapdelete(t *maptype, h *hmap, key unsafe.Pointer)` is similar to the set operation.
 It will check concurrency, evacuate affected bucket if growing and find and delete key
 value pair.

It also tries to update tophash flag: emptyOne and emptyRest.

### zero value
While reading the source code, you may find lots of code like following:

    return unsafe.Pointer(&zeroVal[0]), false

And the `zeroVal` is defined as:

```go
const maxZero = 1024 // must match value in cmd/compile/internal/gc/walk.go
var zeroVal [maxZero]byte
```

Then why 1024, why not 32, 128? What's the magic here? If the key is not found, go will
 return zero value of the value type. So the `zeroVal` needs to hold at least `sizeof(value)`
 bytes of zero.

1024 bytes of zero is enough for moust value type. What about those large than this?
They are handled by `mapaccess1_fat`, `mapaccess2_fat`.

```go
func mapaccess1_fat(t *maptype, h *hmap, key, zero unsafe.Pointer) unsafe.Pointer {
	e := mapaccess1(t, h, key)
	if e == unsafe.Pointer(&zeroVal[0]) {
		return zero
	}
	return e
}
```

The compiler will check size of value and choose proper function to call:

```go
// go:  src/cmd/compile/internal/gc/walk.go
		if w := t.Elem().Width; w <= 1024 { // 1024 must match runtime/map.go:maxZero
			fn := mapfn(mapaccess2[fast], t)
			r = mkcall1(fn, fn.Type.Results(), init, typename(t), r.Left, key)
		} else {
			fn := mapfn("mapaccess2_fat", t)
			z := zeroaddr(w)
			r = mkcall1(fn, fn.Type.Results(), init, typename(t), r.Left, key, z)
		}
```

The zero value used to be an atomic variable that grows at runtime if needed,
 [commit 60fd32](https://github.com/golang/go/commit/60fd32a47fdffb95d3646c9fc75acc9beff67183)
 optimized it to avoid lock and atomic variable.

### references
- [go source: runtime/map.go](https://github.com/golang/go/blob/go1.13.5/src/runtime/map.go)
