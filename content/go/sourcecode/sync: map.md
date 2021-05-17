```metadata
tags: go, sourecode, sync, map
```

## go sync: map

We know that the runtime map is not concurrent safe. So if we need to share map in
 multiple goroutines, we need to protect it using `sync.Mutex` or `sync.Map`. But
 what's the difference?

According to the official document, `sync.Map` is suggested in following cases:

    // The Map type is optimized for two common use cases: (1) when the entry for a given
    // key is only ever written once but read many times, as in caches that only grow,
    // or (2) when multiple goroutines read, write, and overwrite entries for disjoint
    // sets of keys. In these two cases, use of a Map may significantly reduce lock
    // contention compared to a Go map paired with a separate Mutex or RWMutex.

### summary
- the runtime map is safe to read concurrently (no growing or other change during reading)
- `sync.Map` uses a `entry` struct to reference value of a key to achieve following effects:
    * only field of entry id changed during updating, address of entry is not changed
    * for deleting, the key and entry itself are not changed so that there is no write to the map
- use atomic operations to load/store entry from the promoted map to avoid mutex lock

### sync.Map data structure
The `read` is a atomic value that referenced to the `readOnly` struct. The `readOnly`
 has a promoted read-only map and an `amended` field which is a hint of whether the
 promoted map is amended by dirty map.

The `misses` counts misses of the read-only map and if it reaches enough value, a new
 map will be generated and promoted as new read-only map.

```go
type Map struct {
	mu Mutex

	// read contains the portion of the map's contents that are safe for
	// concurrent access (with or without mu held).
	//
	// The read field itself is always safe to load, but must only be stored with
	// mu held.
	//
	// Entries stored in read may be updated concurrently without mu, but updating
	// a previously-expunged entry requires that the entry be copied to the dirty
	// map and unexpunged with mu held.
	read atomic.Value // readOnly

	// dirty contains the portion of the map's contents that require mu to be
	// held. To ensure that the dirty map can be promoted to the read map quickly,
	// it also includes all of the non-expunged entries in the read map.
	//
	// Expunged entries are not stored in the dirty map. An expunged entry in the
	// clean map must be unexpunged and added to the dirty map before a new value
	// can be stored to it.
	//
	// If the dirty map is nil, the next write to the map will initialize it by
	// making a shallow copy of the clean map, omitting stale entries.
	dirty map[interface{}]*entry

	// misses counts the number of loads since the read map was last updated that
	// needed to lock mu to determine whether the key was present.
	misses int
}

// readOnly is an immutable struct stored atomically in the Map.read field.
type readOnly struct {
	m       map[interface{}]*entry
	amended bool // true if the dirty map contains some key not in m.
}

// expunged is an arbitrary pointer that marks entries which have been deleted
// from the dirty map.
var expunged = unsafe.Pointer(new(interface{}))

// An entry is a slot in the map corresponding to a particular key.
type entry struct {
	// p points to the interface{} value stored for the entry.
	//
	// If p == nil, the entry has been deleted and m.dirty == nil.
	//
	// If p == expunged, the entry has been deleted, m.dirty != nil, and the entry
	// is missing from m.dirty.
	p unsafe.Pointer // *interface{}
}
```

### atomic and double check
It's common to see code like following. It loads the atomic value again after gets
 the mutex lock. The reason is that the atomic value may change during the gap of
 the first loading and getting the mutex lock.

```go
	read, _ := m.read.Load().(readOnly)
	e, ok := read.m[key]
	if !ok && read.amended {
		m.mu.Lock()
		// Avoid reporting a spurious miss if m.dirty got promoted while we were
		// blocked on m.mu. (If further loads of the same key will not miss, it's
		// not worth copying the dirty map for this key.)
		read, _ = m.read.Load().(readOnly)
		e, ok = read.m[key]
        //// ....
		m.mu.Unlock()
	}
```

### load
For the loading, it will try to get entry from the read-only map. If found, it returns
 entry value directly.

If not found, it will try to find from the dirty map. And it needs the mutex lock.

Then we know that it is very fast if the key is existed in the read-only map since
 it doesn't need to acquire the mutext lock.

Attention, each time we got a key from the dirty map, the `misses` counter will increase.
 And it will promote the dirty map to read-only map and reset the dirty map and counter
 if it reaches length of the dirty map.

```go
func (m *Map) Load(key interface{}) (value interface{}, ok bool) {
	read, _ := m.read.Load().(readOnly)
	e, ok := read.m[key]
	if !ok && read.amended {
		m.mu.Lock()
		// Avoid reporting a spurious miss if m.dirty got promoted while we were
		// blocked on m.mu. (If further loads of the same key will not miss, it's
		// not worth copying the dirty map for this key.)
		read, _ = m.read.Load().(readOnly)
		e, ok = read.m[key]
		if !ok && read.amended {
			e, ok = m.dirty[key]
			// Regardless of whether the entry was present, record a miss: this key
			// will take the slow path until the dirty map is promoted to the read
			// map.
			m.missLocked()
		}
		m.mu.Unlock()
	}
	if !ok {
		return nil, false
	}
	return e.load()
}

func (m *Map) missLocked() {
	m.misses++
	if m.misses < len(m.dirty) {
		return
	}
	m.read.Store(readOnly{m: m.dirty})
	m.dirty = nil
	m.misses = 0
}
```

### store
It's very fast if the key is existed in the read-only map since it only updates field
 of the entry.

If not found, it needs to acquire the lock and write to the dirty map.

If the read-only map is not amended which means that the dirty map is nil, then it will
 call `m.dirtyLocked()` to make a new dirty map and copy all valid key value pairs to
 the dirty map and mark the read-only map as amended.

```go
func (m *Map) Store(key, value interface{}) {
	read, _ := m.read.Load().(readOnly)
	if e, ok := read.m[key]; ok && e.tryStore(&value) {
		return
	}

	m.mu.Lock()
	read, _ = m.read.Load().(readOnly)
	if e, ok := read.m[key]; ok {
		if e.unexpungeLocked() {
			// The entry was previously expunged, which implies that there is a
			// non-nil dirty map and this entry is not in it.
			m.dirty[key] = e
		}
		e.storeLocked(&value)
	} else if e, ok := m.dirty[key]; ok {
		e.storeLocked(&value)
	} else {
		if !read.amended {
			// We're adding the first new key to the dirty map.
			// Make sure it is allocated and mark the read-only map as incomplete.
			m.dirtyLocked()
			m.read.Store(readOnly{m: read.m, amended: true})
		}
		m.dirty[key] = newEntry(value)
	}
	m.mu.Unlock()
}
```

### delete
The `Delete` simply calls the `LoadAndDelete`. It will try to update the entry value
 to `nil` using atomic operation if the key is valid.

Otherwise, it will lock and delete it from the dirty map.

### promotion
From the loading part, we know that the promotion may happen during the loading of a
 key (Load, LoadOrStore, LoadAndDelete).

It will record cache missing time (counts loading from dirty map). If the counter is
 equal or large than dirty map length, the dirty map is promoted.

So for case that new keys were added and loaded frequently, there will have too much
 promotion and map copying. So that `sync.Map` is not suitable for this case.

### expunged
There is a `expunged` pointer that marks the key as deleted. But why it and what's
 the difference between `expunged` and `nil`?

From the `tryStore` we can find that the `tryStore` can update entry will nil pointer
 but it cannot do fast updating with `expunged` entry.

```go
var expunged = unsafe.Pointer(new(interface{}))

// tryStore stores a value if the entry has not been expunged.
//
// If the entry is expunged, tryStore returns false and leaves the entry
// unchanged.
func (e *entry) tryStore(i *interface{}) bool {
	for {
		p := atomic.LoadPointer(&e.p)
		if p == expunged {
			return false
		}
		if atomic.CompareAndSwapPointer(&e.p, p, unsafe.Pointer(i)) {
			return true
		}
	}
}
```

A entry is set to `expunged` only in `tryExpungeLocked()` and this is called during
 `m.dirtyLocked()`. It will move all valid keys from read-only map to the newly created
  dirty map. For nil entry, it won't copy and it will update it to `expunged` entry.

But why? The reason is that there may have other goroutines that may update the nil entry
 to valid value after this key is checked in `dirtyLocked()` and then this new valid key
 won't be moved into dirty map. After the dirty map is promoted, this key will be disappeared.

To avoid this, it marks nil entry to `expunged` entry so that other goroutines cannot
 update it via `tryStore`. They have to get the lock and write into the dirty map.

### misunderstanding about concurrently writing
Lots of people think that `sync.Map` is suitable for case that reading is far more frequent
 than writing and you should not use it for case that write a lot.

The first half of the sentence is right. But for the second half, it depends. It could
 have high performance if the updating happens on existed keys mostly. For this case, the
 map is not changed, only field of entry is updated.

### references
- [go source: sync map](https://github.com/golang/go/blob/go1.16.4/src/sync/map.go)
