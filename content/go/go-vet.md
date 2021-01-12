```metadata
tags: go, lint
```

## go vet

The `go vet` is the builtin compile time code checking tool. It likes eslint of js.

Some tips

    - go vet is not forcible, some ide may help to run it automatically
    - go vet is not for styles, it is more about real errors

### example
For following code, you can build and run it without problems. But if you run `go vet`
 it will give you some notices.

```go
func main() {
	a := sync.Mutex{}
	arr1 := []int{1,2,3,4}
	for i,v := range arr1 {
		go func() {
			fmt.Println(i,v)
		}()
	}
	fmt.Printf("a is %v\n", a)
}

prog.go:15: loop variable i captured by func literal
prog.go:15: loop variable v captured by func literal
prog.go:18: call of fmt.Printf copies lock value: sync.Mutex
```
It warns you that the result of closure in loop may not be what you want. And `Mutex`
 cannot be copied.

### inside
The `go/src/cmd/vendor/golang.org/x/tools/go/analysis/passes` has a lot of checks, like
 `loopclosure` and `copylock` which showed above.

`go vet` will check all of them.

