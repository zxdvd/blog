```metadata
tags: go, sourecode, sync
```

## go sync: wait group

A `WaitGroup` waits for a collection of goroutines to finish. Following is an example
 of `WaitGroup` in kubernetes. It uses 2 goroutines to do duplex copy and waits for
 finishing of the 2 goroutines.

Attention, the `copyBytes` function receives the `&wg` as parameter and it will call
 `wg.Done()` before exit.

```go
// kubernetes: pkg/proxy/userspace/proxysocket.go
// ProxyTCP proxies data bi-directionally between in and out.
func ProxyTCP(in, out *net.TCPConn) {
	var wg sync.WaitGroup
	wg.Add(2)
	klog.V(4).Infof("Creating proxy between %v <-> %v <-> %v <-> %v",
		in.RemoteAddr(), in.LocalAddr(), out.LocalAddr(), out.RemoteAddr())
	go copyBytes("from backend", in, out, &wg)
	go copyBytes("to backend", out, in, &wg)
	wg.Wait()
}

func copyBytes(direction string, dest, src *net.TCPConn, wg *sync.WaitGroup) {
	defer wg.Done()
    // ...
}
```

Above is use case of WaitGroup. Following will dig into the internal to learn the
 implementation of the WaitGroup.

The WaitGroup has a `noCopy` which is an empty struct that used for linting. The
 `go vet` will check this to avoid copy of WaitGroup.

```go
type WaitGroup struct {
	noCopy noCopy

	// 64-bit value: high 32 bits are counter, low 32 bits are waiter count.
	// 64-bit atomic operations require 64-bit alignment, but 32-bit
	// compilers do not ensure it. So we allocate 12 bytes and then use
	// the aligned 8 bytes in them as state, and the other 4 as storage
	// for the sema.
	state1 [3]uint32
}
```

It seems it only use 3 uint32. There is a lot of explain about it. We can just
 take is as following:

```go
type WaitGroup struct {
	noCopy noCopy
    v      uint32       // the counter, sum of wg.Add(n)
    w      uint32       // the waiter counter
    sema   uint32       // the semaphore underground
}
```

`v` and `w` are read/write atomically together as a uint64. `v`, `w` and `sema`
 are mixed together since `v` and `w` require 64 bit alignment.

The core logic of WaitGroup is like following:

- v is added by `N` when you call `wg.Add(N)`
- w is added by one when `wg.Wait()` is called
- v is substracted by 1 when `wg.Done()` is called (same as `wg.Add(-1)`)
- when v == 0, it means all goroutines are finished, it will release sema `w` times
 so that all waiting goroutines will be signalled and ready to run

Core code of `wg.Add`:

```go
func (wg *WaitGroup) Add(delta int) {
	state := atomic.AddUint64(statep, uint64(delta)<<32)
	v := int32(state >> 32)
	w := uint32(state)
	if v < 0 {
		panic("sync: negative WaitGroup counter")
	}
	if w != 0 && delta > 0 && v == int32(delta) {
		panic("sync: WaitGroup misuse: Add called concurrently with Wait")
	}
	if v > 0 || w == 0 {
		return
	}
	// Reset waiters count to 0.
	*statep = 0
	for ; w != 0; w-- {
		runtime_Semrelease(semap, false, 0)
	}
}
```

Following is the code of `wg.Wait()`. It adds `w` counter by 1 and requires the sema
 so that this goroutine will go to wait state.

```go
// Wait blocks until the WaitGroup counter is zero.
func (wg *WaitGroup) Wait() {
	statep, semap := wg.state()
	for {
		state := atomic.LoadUint64(statep)
		v := int32(state >> 32)
		w := uint32(state)
		if v == 0 {
			// Counter is 0, no need to wait.
			return
		}
		// Increment waiters count.
		if atomic.CompareAndSwapUint64(statep, state, state+1) {
			runtime_Semacquire(semap)
			if *statep != 0 {
				panic("sync: WaitGroup is reused before previous Wait has returned")
			}
			return
		}
	}
}
```

The `wg.Done()` just decrements v counter by 1.

```go
// Done decrements the WaitGroup counter by one.
func (wg *WaitGroup) Done() {
	wg.Add(-1)
}
```

### references
- [go 1.16: sync - WaitGroup](https://github.com/golang/go/blob/go1.16.4/src/sync/waitgroup.go)
