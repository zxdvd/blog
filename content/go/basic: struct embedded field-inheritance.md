```metadata
tags: go, spec, inheritance
```

## basic: go struct embedded field
Unlike objected oriented languages like python, java, go doesn't have features like
 class, class inheritance currently.

Go struct supports embedded field that likes inheritance but actually not.

### fields or methods of embedded field can be promoted
For following code snippet, `c1.Name()` actually calls `Name()` of `parent`.

```go
type parent struct {
	name string
}

func (p *parent) Name() string {
	return "parent name: "+ p.name
}

type child struct {
	parent
	Age int
}

func main() {
	c1 := child{}
	fmt.Println(c1, c1.Name())        // got "{{} 0} parent name: "
}
```

### fields or methods of embedded can be overwritten
For same code, let's add `Name()` for type `child`.

```go
func (c child) Name() string {
	return "child name: "+ c.name
}
```

The result is `{{} 0} child name: `. Then we know that `child.Name()` overwrites
 `parent.Name()`.

Can I use the overwritten fields or methods in extended struct?

Of course, just use it explicitly with embedded field name, like following:

```go
func (c child) Name() string {
	pname := c.parent.Name()
	return pname + "\nchild name: "+ c.name
}
```

### can parent call or use fields or methods of child
You may write code like following that try to call method of **child** in **parent**
 struct. However, you'll get compiling error
 `p.Name undefined (type *parent has no field or method Name)`.

Then we know that: we CANNOT use fields or methods of child in parent.

Actually, it's easy to guess the result. From above section, we know that `c1.FullName()`
 refers to `c1.parent.FullName()` and `c1.parent` doesn't have a method `Name()`.

```go
type parent struct {}

func (p *parent) FullName() string {
	return "full name is: "+ p.Name()         // expect to call Name() of child
}

type child struct {
	parent
	firstName string
	lastName string
}

func (c child) Name() string {
	return fmt.Sprint(c.firstName, " ", c.lastName)
}

func main() {
	c1 := child{
		firstName: "James",
		lastName: "Green",
	}
	fmt.Println(c1, c1.FullName())
}
```

Attention, it's nice that we got a compiling error here. If you occasionally implemented
 `Name()` for type `parent`, then compiler won't complain and things got worse since your
 code will build and run with bugs.


### what about multiple children have same methods or fields
For following code, `child` extended from both `parent1` and `parent2` which both implement
 `Name()`.

```go
type parent1 struct {}

func (p parent1) Name() string {
	return "parent1"
}

type parent2 struct {}
func (p parent2) Name() string {
	return "parent2"
}

type child struct {
	parent1
	parent2
}
```

What will happen if you calls `c1.Name()`?

You'll get error `ambiguous selector c1.Name`. It's easy to understand, compiler doesn't
 know which `Name()` to select.

You can implement `child.Name` explicitly to avoid this.


### be careful of `runtime error: nil pointer dereference`
The `runtime error: nil pointer dereference` is a common error in go. And it's not limit to
 embedded struct.
 Following is a demo code that will lead to this panic error:

```go
type if1 interface {
	Name() string
}

type child struct {
	if1
}

func main() {
	c1 := child{}
	fmt.Println(c1, c1.Name())
}
```

You can compile it. But you'll get panic when running it. It's understandable. Type `child`
 has an interface `if1` as embedded field. An interface is a collection of virtual methods.
 A lot of types can implement a same interface. So you don't know the actually type of the
 interface before initializing it. Without an explicit type, you don't know the actually
 address of a method or a field.

Following code can run without panic even though `tmp` is nil.

```go
func main() {
    var tmp *parent     // type parent implement if1 interface
    c1 := child{ if1: tmp }
	fmt.Println(c1, c1.Name())
}
```

Finally, just remeber that embedded struct is not inheritance and forget class, subclass,
 inheritance.

### references
- [golang spec: struct types](https://golang.org/ref/spec#Struct_types)
