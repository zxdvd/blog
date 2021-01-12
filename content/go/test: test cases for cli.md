```metadata
tags: go, test, flag
```

## go test: test cases for cli application

It's very good to write test cases and it's easy to write test cases for a package.
 However, have you thought about how to write test cases for the `main` package?
 Especially when you want to test the command line arguments.

One way is keeping your arguments parsing layer thin and small. Write most logic
 in another function like `app(options)`. And just test the function `app`. It's
 actually not `end-to-end` test of the whole application. But since the arguments
 related part is very thin and we believe that there is no bug in the offical `flag`
 package. Then this is an option.

I used to try another way. Write the `main.go` like following:

```go
func main() {
	Main()
}

func Main() {
    // argument parsing and app runnning
}
```

By this way, I wrote test case like following:

```go
func TestMConfig(t *testing.T) {
	os.Args = []string{"", "-debug", "--config", "testdata/tasks.yml"}
	Main()
}
```

You can set `os.Args` with any command line options you want to test it. However,
 this has a big caveat when I tried to run another test cases. I got error like
 `/var/folders/xxxxx/main.test flag redefined: config`. It's easy to understand.
 I use `flag.String("config", "", "the config path")` in `Main()`. When `Main()`
 is called in second test case, the `-config` is refined. I didn't find a way
 to clear the state of `flag`. But we can use `flag.NewFlagSet()` to create a
 new, empty flag set. So the arguments parsing code need to change a little, like:

```go
func Main() {
	f := flag.NewFlagSet("my tools", flag.ExitOnError)
	configfile := f.String("config", "", "config file path")
	f.Parse(os.Args[1:])
}
```

By this way, each time you call `Main()` it will create a empty flag set. And
 you can write multiple test cases and won't get the `flag redefined` error.

I also find another way to test command line application, you can build the
 application and store the binary in an temporary folder. And use `exec.Command`
 to run it with arguments you want to test. This comes closest to real world
 scenario. But it is a little complicated. I find some go command line tools
 use this way to test.

