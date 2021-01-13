```metadata
tags: go, package, module
```

## basic: initialization

### variable initialization
Expressions and functions are initialized from left to right if they are appreared at
 same line and they have no dependencies.

Variables in a same package are initialized in dependency order. If `a` depends on `b`,
 then `a` will initialize after `b`. Variables with no dependency are initialized first.

Example from the [spec: order of evaluation](https://golang.org/ref/spec#Order_of_evaluation):

```go
var a, b, c = f() + v(), g(), sqr(u()) + v()

func f() int        { return c }
func g() int        { return a }
func sqr(x int) int { return x*x }

// functions u and v are independent of all other variables and functions

The function calls happen in the order u(), sqr(), v(), f(), v(), and g().
```

Since `f()` depends on `c`, `a = f() + v()` is initialized later.

Multiple variables declared together are initialized together. Following is another
 example from the [spec](https://golang.org/ref/spec#Package_initialization):

```go
var x = a
var a, b = f() // a and b are initialized together, before x is initialized
```


### init function
A package can have multiple `func init()`, even in same file. `init` functions are
 executed after initialization of all package level variables. They are called in
 the order the appear in the source. For `init` in multiple files, the spec says

    To ensure reproducible initialization behavior, build systems are **encouraged**
    to present multiple files belonging to the same package in lexical file name order
    to a compiler.

This is not a force requirement, I suggest that we'd not writing code that depend on the
 order of the `init` executing.


### package initialization
If a package imports multiple other packages, then all imported packages are initialized
 at first, then the package itself. If multiple packages import a same package `A`, `A`
 is guaranteed to be initialized only once.

Golang doesn't support cyclic importing, the compiling will fail if packages have cyclic
 dependencies.

### references
- [golang spec: program initialization](https://golang.org/ref/spec#Program_initialization_and_execution)
