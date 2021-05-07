```metadata
tags: go, sourecode, runtime, map, hashmap
```

## go runtime: map part1

`map` or `dict` is very basic data container that almost every language supports it.
`map` is often implemented using [hash table](../../data_structure/hash table.md).

I used to read source code of `HashMap` of java. It's just very common java code.
 I can understand it even though I didn't learn java. It's a chained hash table that
 each bucket is a linked list or red-black tree (when longer than a threshold).

The `map` of go is a little complicated. It's implemented in the runtime package
 since go doesn't have generics support so that it needs help of compiler and
 runtime.

And it grows gradually like redis while java and many other languages grow at once.
By this way, operations on `map` won't block for a long time when it is resize and
 average time is more evenly compared with those resize at once. The bad side is
 that it adds compelixity.

Go also uses chained hash map, but more complicated. Each hash slot is linked
 buckets. But each bucket holds `8` key value pairs instead of 1. The 8 key value
 pairs are packed together so that it's quick to loop through them. That's why
 it uses a large load factor, 6.5, while java is 0.75. Considering that 8 KV
 packed together, the load factor is similar to 6.5/8 (~0.81).

### data structure
A `map` is represented by `hmap`:

```go
type hmap struct {
	count     int // # live cells == size of map.  Must be first (used by len() builtin)
	flags     uint8
	B         uint8  // log_2 of # of buckets (can hold up to loadFactor * 2^B items)
	noverflow uint16 // approximate number of overflow buckets; see incrnoverflow for details
	hash0     uint32 // hash seed

	buckets    unsafe.Pointer // array of 2^B Buckets. may be nil if count==0.
	oldbuckets unsafe.Pointer // previous bucket array of half the size, non-nil only when growing
	nevacuate  uintptr        // progress counter for evacuation (buckets less than this have been evacuated)

	extra *mapextra // optional fields
}
```

Most of them are very easy to understand. `count` is total size. `B` is log2 of bucket size.
It has `buckets` and `oldbuckets` since it grows gradually but not at oneshot. `hash0` is
 seed of hash function to avoid hash attack.

Each bucket is a `bmap`:

```go
type bmap struct {
	// tophash generally contains the top byte of the hash value
	// for each key in this bucket. If tophash[0] < minTopHash,
	// tophash[0] is a bucket evacuation state instead.
	tophash [bucketCnt]uint8
	// Followed by bucketCnt keys and then bucketCnt elems.
	// NOTE: packing all the keys together and then all the elems together makes the
	// code a bit more complicated than alternating key/elem/key/elem/... but it allows
	// us to eliminate padding which would be needed for, e.g., map[int64]int8.
	// Followed by an overflow pointer.
}
```

The actual memory layout of `bmap` is like following:

```
    tophash 8bytes
    [key0] [key1] [key2] [key3] [key4] [key5] [key6] [key7]
    [val0] [val1] [val2] [val3] [val4] [val5] [val6] [val7]
    overflow pointer to next bucket
```

The 8 bytes `tophash` is used to find index of key value pair quickly. The highest byte
 of hash of the key stored in this bucket is also stored in the tophash. You can check
 the ith byte of `tophash` with the hash first, you don't need to waste time to compare
 the whole key if the top byte is not matched. Comparing 1 byte is faster than multiple
 bytes keys, especially when the key is a indirect pointer.

When the bucket is full, you need to chain with another, so you need a pointer to point
 to next bucket, that's the `overflow` pointer at the end of the bucket.

The `hmap` also has a `mapextra` field:

```go
type mapextra struct {
	// If both key and elem do not contain pointers and are inline, then we mark bucket
	// type as containing no pointers. This avoids scanning such maps.
	// However, bmap.overflow is a pointer. In order to keep overflow buckets
	// alive, we store pointers to all overflow buckets in hmap.extra.overflow and hmap.extra.oldoverflow.
	// overflow and oldoverflow are only used if key and elem do not contain pointers.
	// overflow contains overflow buckets for hmap.buckets.
	// oldoverflow contains overflow buckets for hmap.oldbuckets.
	// The indirection allows to store a pointer to the slice in hiter.
	overflow    *[]*bmap
	oldoverflow *[]*bmap

	// nextOverflow holds a pointer to a free overflow bucket.
	nextOverflow *bmap
}
```

Go may allocate more buckets than needed and these extra buckets will be used for
 overflow buckets. The `nextOverflow` points to begin of free overflow bucket.

The source code has a large piece comment about `overflow` and `oldoverflow`.
For the case that both key and value are not pointers, it needs to store the
 overflow buckets here to protect it from reclaiming by garbage collector since
 the overflow bucket is not defined in `bmap` (bmap is allocated and managed
 using raw memory) and nothing references it. However, if key or value is pointer,
 it won't be reclaimed since `type maptype struct` has a bucket field that references
 to it. Then we don't need the two containers here at this case.

### constant values
The load factor is defined using two integers to avoid float calculation. As explained
 at the beginning, you can think it as `6.5/8=0.8125` too since each bucket has 8
 fixed key value pairs.

```go
	loadFactorNum = 13
	loadFactorDen = 2
```

The `bucketCnt` determines how many key value pairs a bucket can hold.

```go
	// Maximum number of key/elem pairs a bucket can hold.
	bucketCntBits = 3
	bucketCnt     = 1 << bucketCntBits
```

The following two are threshold to determine that whether key/value should be stored
 in the bucket directly or referenced by a pointer.

```go
	maxKeySize  = 128
	maxElemSize = 128
```

We know that each byte in `tophash` is used to store highest byte of hash of the key
 to help locate index of KV pair quickly.

However, go also uses it to store flags of each slot in the bucket. `0` and `1` are
 used to indicate that related slot is empty, `0` indicates all following are empty.
`2`, `3` and `4` store state about evacuating (move from old bucket to new bucket).

```go
	emptyRest      = 0 // this cell is empty, and there are no more non-empty cells at higher indexes or overflows.
	emptyOne       = 1 // this cell is empty
	evacuatedX     = 2 // key/elem is valid.  Entry has been evacuated to first half of larger table.
	evacuatedY     = 3 // same as above, but evacuated to second half of larger table.
	evacuatedEmpty = 4 // cell is empty, bucket is evacuated.
	minTopHash     = 5 // minimum tophash for a normal filled cell.
```

You may wonder how to deal with overlapping of the highest byte of hash. Go simply
 add `minTopHash` if the highest byte is small than `minTopHash`. It's ok to do
 this since this byte is only hint byte help to locate key value index.

```go
func tophash(hash uintptr) uint8 {
	top := uint8(hash >> (sys.PtrSize*8 - 8))
	if top < minTopHash {
		top += minTopHash
	}
	return top
}
```

### references
- [go source: map](https://github.com/golang/go/blob/go1.13.5/src/runtime/map.go)
