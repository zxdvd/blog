<!---
tags: c, macro
-->

## multiple statements macro
It's very common to have multiple statements in macro. Following are codes from famous open 
source projects.

```c
// redis: src/dict.h
#define dictSetKey(d, entry, _key_) do { \
    if ((d)->type->keyDup) \
        (entry)->key = (d)->type->keyDup((d)->privdata, _key_); \
    else \
        (entry)->key = (_key_); \
} while(0)

// postgres: src/include/storage/lock.h
#define SetInvalidVirtualTransactionId(vxid) \
	((vxid).backendId = InvalidBackendId, \
	 (vxid).localTransactionId = InvalidLocalTransactionId)

// postgres: src/backend/commands/analyze.c
#define swapInt(a,b)	do {int _tmp; _tmp=a; a=b; b=_tmp;} while(0)
```

It's easy to understand why not easily write as `#define swapInt(a,b)	int _tmp; _tmp=a; a=b; b=_tmp`. 
A simple example:

```c
if (do_swap)
    swapInt(a.val, b.val);

// let's expand the macro, will get
if (do_swap)
    int _tmp; _tmp = a.val; a.val = b.val; b.val = _tmp;
// format code
if (do_swap)
    int _tmp;
_tmp = a.val;
a.val = b.val;
b.val = _tmp;
```

Let's just ignore the `int _tmp` declare error. It actually changes the logical. Only the first statement 
is the body of the `if` condition. All following will be executed normally which is not intended. And you 
may get compile error if there is a `else` here.

So that we need to wrap them into a code block. Then you may wonder why not wrap with `{}` block? Why 
`do while(0)`? Let's see following code.

```c
#define swapInt(a,b)	{int _tmp; _tmp=a; a=b; b=_tmp;}
if (do_swap)
    swapInt(a.val, b.val);
else
    printf("i am in else");

// expand it will get
if (do_swap)
{
    int _tmp;
    _tmp = a.val;
    a.val = b.val;
    b.val = _temp;
};
else
    printf("i am in else");
```

There is a `;` after the `{}` block. It's end of the if condition. Then you'll get compile error 
`error: ‘else’ without a previous ‘if’`. You can replace `swapInt(a.val, b.val);` with 
`swapInt(a.val, b.val)`, remove the trailing semicolon.

However, it's weird and error-prone. And the `do{} while(0)` avoids this problem. It's a statement 
that you should add `;` and it has a inside code block then you can write multiple statements.

In the beginning example, there is another style that concatenates multiple statements with `,`.
It's ok but not so popular as the `do {} while(0)` for the following reasons:

- it do not have code blocks, so that you cannot decare variables like `int _tmp`.
- cannot use complex control syntax like `if else`.



