```metadata
tags: go, sourecode, runtime, map, hashmap
```

## go runtime: map part3 - growing

As explained in previous article, go map uses gradually resizing like redis. So how
 does it work?

Go implements a double size resizing and a same size resizing. There is no shrinking.
It's easy to understand double size resizing. But why same size resizing?

Since go uses chained bucket and each bucket has 8 slots. Then you may get following
 scenario:

- you add a lot of KV pairs and a lot of overflow buckets created but load
 factor is not enough to trigger double resize

- then you remove some keys and it leads to lots of empty slots in direct buckets (non-overflow bucket)

For this case, a same size growing can help to eliminate some overflow buckets by move
 data to direct buckets. This can improve performance.

### data structure about growing

### when to resize
Resizing only triggered when you want to add a new KV pair into the map, thus in `mapassign`.

```go
	if !h.growing() && (overLoadFactor(h.count+1, h.B) || tooManyOverflowBuckets(h.noverflow, h.B)) {
		hashGrow(t, h)
		goto again // Growing the table invalidates everything, so try again
```

When `mapassign` failed to find an existed key and it wasn't growing, then it will check
 load factor and whether there is too much overflow buckets, if any is true, it will start
 growing.

The `hashGrow` set flag to indicate whether it's double size resizing or same size resizing.
And creates new buckets and does some initiation.

The `hashGrow` just does preparation, it won't do any actual data moving. 
```go
func hashGrow(t *maptype, h *hmap) {
	bigger := uint8(1)
	if !overLoadFactor(h.count+1, h.B) {
		bigger = 0
		h.flags |= sameSizeGrow
	}
	oldbuckets := h.buckets
	newbuckets, nextOverflow := makeBucketArray(t, h.B+bigger, nil)

	flags := h.flags &^ (iterator | oldIterator)
	if h.flags&iterator != 0 {
		flags |= oldIterator
	}
	// commit the grow (atomic wrt gc)
	h.B += bigger
	h.flags = flags
	h.oldbuckets = oldbuckets
	h.buckets = newbuckets
	h.nevacuate = 0
	h.noverflow = 0
}
```

### when to grow

The resizing stage won't do any data moving.  The actual moving happens in each
 set/delete (update) operation gradually. Following code is called to move affected
 bucket immediately so that it needn't to deal with old bucket.

``` go
	bucket := hash & bucketMask(h.B)
	if h.growing() {
		growWork(t, h, bucket)
	}
```

Does it mean that it only moves affected bucket? How about those never touched?
Actually, the `growWork` function will the affected bucket first and then do an
 extra bucket moving from bucket 0 sequentially.

``` go
func growWork(t *maptype, h *hmap, bucket uintptr) {
	evacuate(t, h, bucket&h.oldbucketmask())

	// evacuate one more oldbucket to make progress on growing
	if h.growing() {
		evacuate(t, h, h.nevacuate)
	}
}
```

Thinking:

    I personally think it may not be a good idea that grows only during updating.
    If you load data into a map and seldomly changed the map after loading, and
    the map is in growing state, then it will be in this state for long time and
    it uses too much memory since both old buckets and new buckets co-exist.

    Maybe it should also grow during accessing.


### how to grow gradually
The `evacuate(t *maptype, h *hmap, oldbucket uintptr)` is used to move all KV pairs
 includes those in overflow buckets of a specific bucket number to new buckets.

Since bucket size is power of 2, only highest bit may change after double sized. Then
 a KV pair either goes to the lower half of the new bucket or higher half. Then it
 defines the `var xy [2]evacDst` to hold the lower half and higher half. The higher
 half is only initialized when it is a double size resizing.

Then it will loop over all KV pairs of the bucket and it's overflow buckets, add each
 KV pair to one of the two halves according to key hash. A special case is dealing
 with the float value `NaN`. According to float number standard, `NaN` is not equalled
 to `NaN`, then `NaN` can be sent to either half. Go chooses to use lowest bit of tophash.

Then it will copy KV to new bucket and set flag at old bucket to mark it as evacuated.

The `advanceEvacuationMark` function is called to skip those evacuated buckets (higher
 bucket number may grow first when it was changed). And it also handles the finishing of
 the whole resizing.

```go
func evacuate(t *maptype, h *hmap, oldbucket uintptr) {
	b := (*bmap)(add(h.oldbuckets, oldbucket*uintptr(t.bucketsize)))
	newbit := h.noldbuckets()
	if !evacuated(b) {
		// xy contains the x and y (low and high) evacuation destinations.
		var xy [2]evacDst
		x := &xy[0]
		x.b = (*bmap)(add(h.buckets, oldbucket*uintptr(t.bucketsize)))
		x.k = add(unsafe.Pointer(x.b), dataOffset)
		x.e = add(x.k, bucketCnt*uintptr(t.keysize))

		if !h.sameSizeGrow() {
			// Only calculate y pointers if we're growing bigger. Otherwise GC can see bad pointers.
			y := &xy[1]
            // init y.b y.k y.e
		}

		for ; b != nil; b = b.overflow(t) {
			k := add(unsafe.Pointer(b), dataOffset)
			e := add(k, bucketCnt*uintptr(t.keysize))
			for i := 0; i < bucketCnt; i, k, e = i+1, add(k, uintptr(t.keysize)), add(e, uintptr(t.elemsize)) {
				top := b.tophash[i]
				if isEmpty(top) {
					b.tophash[i] = evacuatedEmpty
					continue
				}
				k2 := k
				if t.indirectkey() { k2 = *((*unsafe.Pointer)(k2)) }
				var useY uint8
				if !h.sameSizeGrow() {
					// Compute hash to make our evacuation decision (whether we need
					// to send this key/elem to bucket x or bucket y).
					hash := t.key.alg.hash(k2, uintptr(h.hash0))
					if h.flags&iterator != 0 && !t.reflexivekey() && !t.key.alg.equal(k2, k2) {
						// If key != key (NaNs), then the hash could be (and probably
						// will be) entirely different from the old hash. Moreover,
						// it isn't reproducible. Reproducibility is required in the
						// presence of iterators, as our evacuation decision must
						// match whatever decision the iterator made.
						useY = top & 1
						top = tophash(hash)
					} else {
						if hash&newbit != 0 {
							useY = 1
						}
					}
				}

				b.tophash[i] = evacuatedX + useY // evacuatedX + 1 == evacuatedY
				dst := &xy[useY]                 // evacuation destination

                // // deal with dst full and move to next slot
			}
		}
	}

	if oldbucket == h.nevacuate {
		advanceEvacuationMark(h, t, newbit)
	}
}
```

### references
- [go source: runtime/map.go](https://github.com/golang/go/blob/go1.13.5/src/runtime/map.go)
