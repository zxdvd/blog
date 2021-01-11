```metadata
tags: js, node
```

## js basic: corners

### syntax
You need to wrap with `()` while doing object destructure with existed variables:

    let { a } = { a:100 }
    { a } = { a: 200 }           // syntax error
    ( { a } = { a: 200 } )       // correct. you need to wrap with ()

#### destructure

### string
#### string replace
Following is syntax of string replace:

    str.replace(regexp|substr, newSubstr|function)

If you use string but not regexp for first parameter, then remember that `str.replace`
 only replace ONCE. See below example:

    'aaa'.replace('a', 'A')   // got Aaa but not AAA

If you want to replace all, you should use regexp and the `g` flag, like below:

    'aaa'.replace(/a/g, 'A')   // got AAA

Another trick in `str.replace` is that js adds some magics in the replacement string.
It will convert `$$` to `$`, `$&` to the matched part and some other magics. For example:

    '##abc##'.replace(/##/g, '$$')  // $abc$ but not $$abc$$

To avoid these magics, you can use a function instead of replacement string, like following:

    '##abc##'.replace(/##/g, () => '$$')  // got $$abc$$

I think the function style is always preferred especially when the replacement string
 is user input.


### comparison (Object.is, ===)
Javascript's `Object.is` is different from python's. For python, `a is b` check whether
 `a` and `b` point to same address while `a == b` does the value compare and it can
 be overridden for user defined class.

However, for javascript, `a is b` is similar to `a === b` with trivial differences.

From MDN, `Object.is()` returns true in following situations:

- both undefined
- both null
- both true or both false
- both strings of the same length with the same characters in the same order
- both the same object (means both object have same reference)
- both numbers and
    + both +0
    + both -0
    + both NaN
    + or both non-zero and both not NaN and both have the same value

It differs from `===` in folllowing situations

- `===` returns true for `-0` and `+0` while `Object.is()` returns false
- `===` treats `NaN` as not equal to `NaN` while `Object.is()` treats them as equal

I prefer to `===` and use `Number.isNaN()` for the `NaN` case.

### check existence of object property
You may find serveral ways to deal with this, like

```js
    o && o.key           // if and only if you're sure that it isn't falsy value
    o && o.key !== undefined       // object may have a key with value `undefined`
    if ('key' in o) {}
    o.hasOwnProperty('key')

    ({}).hasOwnProperty.call(o, 'key')
    Object.prototype.hasOwnProperty.call(o, 'key')
```

### iterator
Check [if object is iterable](https://stackoverflow.com/questions/18884249/checking-whether-something-is-iterable):

```js
function isIterable(obj) {
    if (obj === null || obj === undefined) {
    return false
    }
    return typeof obj[Symbol.iterator] === 'function'
}
```


### references
- [MDN: string replace](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String/replace)
- [MDN: object is](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object/is)
