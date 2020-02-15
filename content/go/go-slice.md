<!---
tags: go, slice
-->

## go slice

### slice copy
You can use the `n := copy(dst, src)` to copy slice. But you need to take care: only 
 `min(len(dst), len(src))` elements would be copied.

Attention: it's **len(dst)** not **cap(dst)**. So for following example, it won't copy
 anything.

```go
    src := []int{1,2,3}
    dst := make([]int, 0, 10)
    n := copy(dst, src)       // n = 0, nothing copied
```

If you want copy slice to a new one, you can also use append:

    dst := append([]int(nil), src...)


### references
[official wiki: SliceTricks](https://github.com/golang/go/wiki/SliceTricks)
