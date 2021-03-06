```metadata
tags: js, node, v8
```

## node show bytecode

Bytecode is intermediate between source code of low level machine code. It is always
 platform independently and more featured and powerful than machine code. It hides
 differences between different architectures.

For node, we can use `node --print-bytecode filename.js` to print the bytecode of a
 source file. However, this will generate a large output even for one line of code.
 I think it has some initiation and runtime. Then you can add `--print-bytecode-filter`
 to filter a function.

```shell
$ node  --print-bytecode  --print-bytecode-filter=f1 filename.js
```

Attention, you won't get any output if the function is just defined but not used anywhere.

A simple example:

```shell
❯ echo $'function f1(a) { return 100+a };f1();' | node --print-bytecode --print-bytecode-filter=f1
[generated bytecode for function: f1]
Parameter count 2
Frame size 8
   11 E> 0x315d135ce22 @    0 : a0                StackCheck
   17 S> 0x315d135ce23 @    1 : 0c 64             LdaSmi [100]
         0x315d135ce25 @    3 : 26 fb             Star r0
         0x315d135ce27 @    5 : 25 02             Ldar a0
   27 E> 0x315d135ce29 @    7 : 32 fb 00          Add r0, [0]
   29 S> 0x315d135ce2c @   10 : a4                Return
Constant pool (size = 0)
Handler Table (size = 0)
```
