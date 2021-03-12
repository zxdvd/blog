```metadata
tags: python, typing
```

## python/inspect: function parameter typing and annotation

Recently, I've tried `fastapi` framework a little since it integrate with openapi
 doc internally so that it can generate a simple openapi documentation site. Looks
 appealing.

With fastapi you can define an api like following:

```py
from fastapi import Request

@app.post('/items/{item_id}')
def create_item(item_id: int, data: BodySchema, req: Request, user_id: int = None):
    pass
```

In above example, fastapi knows that the parameter `item_id` is a path parameter,
 `data` is request body, `req` is the incoming request itself while `user_id` is a
 query string parameter.

But, how did it know these? The framework doesn't require any order of the parameters.
 And python is a dynamic language that it doesn't have strong type reflection support
 like static typing language.

So there must have a method to get typing information of a function.

The type annotation is a new feature from python 3.5. Related PEPs are: PEP 3107 and
 PEP 484.

PEP 3107 introduced **__annotations__** property that has information about parameter
 annotations. So that you can use it to get type hint.

```py
def fn1(a: int, b: str, c: Any, d, e: 'i am a parameter'):
    pass

print('annotations: ', fn1.__annotations__)

// output
annotations:  {'a': <class 'int'>, 'b': <class 'str'>, 'c': typing.Any, 'e': 'i am a parameter'}
```

We can find that **__annotations__** is a dict that holds parameters and related annotations.
Parameter that has no annotation will not exist in this dict.

However, this dict has no information about position of each parameter. So we cannot
 do reflection like fastapi.

The fastapi uses the `inspect.signature(fn)` to get signature of a function. It contains
 ordered parameter informations and annotation of the function return. Like following example:

```py
def fn1(a: int, b: str, c: Any, d, e: 'i am a parameter'):
    pass

sig = inspect.signature(fn1)
print('signature parameters: ', sig.parameters)
for k, v in sig.parameters.items():
    print('parameter: ', v, ', annotation: ', v.annotation)

// output
signature parameters:  OrderedDict([('a', <Parameter "a: int">), ('b', <Parameter "b: str">), ('c', <Parameter "c: Any">), ('d', <Parameter "d">), ('e', <Parameter "e: 'i am a parameter'">)])
parameter:  a: int , annotation:  <class 'int'>
parameter:  b: str , annotation:  <class 'str'>
parameter:  c: Any , annotation:  typing.Any
parameter:  d , annotation:  <class 'inspect._empty'>
parameter:  e: 'i am a parameter' , annotation:  i am a parameter
```

With help of **signature**, fastapi will replace parameter with `Request` (incoming request)
 when it knows that type of the parameter is `Request`.

### references
- [PEP 3107](https://www.python.org/dev/peps/pep-3107/)
- [PEP 484](https://www.python.org/dev/peps/pep-0484/)
