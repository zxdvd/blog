+++
Title = "python: default parameter of function"
Date = "2015-08-05"
Categories = "Python"
Tags = ["python"]
Slug = "python-function-default-parameter"
Authors = "Xudong"
+++

I'll show a small peice of code at first:

    :::python
    >>> def foo(a=[]):
    ...   a.append(1)
    ...   print(a)
    ...
    >>> foo()
    [1]
    >>> foo()
    [1, 1]
    >>> foo()
    [1, 1, 1]
    >>> foo
    <function foo at 0x7efc48708bf8>

I was confused by this when first met it years ago. A lot of people, me
included, may think that list **a** should always be **[1]** and why it behaviors like a
global variable.

#### Everything is an object

We frequently see this quote but it's not easy to understand.

In python, a function is an object too. This object is created after definition.
And the function name is just like a reference to the object. And the default
parameter of the function is also determined at the same time.

Following is official documentation of [function
definitions](https://docs.python.org/3.5/reference/compound_stmts.html#function-definitions)

    :::text
    Default parameter values are evaluated from left to right when the function
    definition is executed. This means that the expression is evaluated once,
    when the function is defined, and that the same “pre-computed” value is used
    for each call. This is especially important to understand when a default
    parameter is a mutable object, such as a list or a dictionary: if the
    function modifies the object (e.g. by appending an item to a list), the
    default value is in effect modified. This is generally not what was
    intended. A way around this is to use None as the default, and
    explicitly test for it in the body of the function.

Summary: default parameter is evaluated only ONCE at the creation of the
function object. It's NOT evaluated when the function is called.

Another piece of code:

    :::python
    >>> def foo(a=[]):
    ...   print('id of a is: ', id(a))
    ...   a.append(1)
    >>> dir(foo)
    ['__annotations__', '__call__', '__class__', '__closure__', '__code__',
    '__defaults__', '__delattr__', '__dict__', '__dir__', ... ... ]
    >>> foo.__defaults__
    ([],)
    >>> foo()
    id of a is:  139897639740424
    >>> foo.__defaults__
    ([1],)
    >>> foo()
    id of a is:  139897639740424
    >>> foo.__defaults__
    ([1, 1],)

We could see that after definition of the function, we have object foo. It has a
lot of attributes just like other classes. And the value of default parameter
are stored at the **\_\_default\_\_**. At first, it has an empty list. It's appended
once the function is called. Moreover, the id of a is not changed while it is
called at the second time.

Now I think we can explain it.

Default parameters are valuated and stored at the \_\_default\_\_ attribute of the
function object. If it is mutable like list, it may be modified (like the
`a.append(1)` in the example) and the modification will be kept.


#### Reference:

1. [Understanding Python's Execution Model](http://www.jeffknupp.com/blog/2013/02/14/drastically-improve-your-python-understanding-pythons-execution-model)
2. [The Mutable Default Argument](http://stackoverflow.com/questions/1132941/least-astonishment-in-python-the-mutable-default-argument)
