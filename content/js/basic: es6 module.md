```metadata
tags: js, node, module
```

## basic: es6 module
Es6 module is the official standard module for js. It uses `import` and `export` statements.
By default it is not enabled, to enable it, you need node >= v12 and either of the following
 two:

- files ending in `.mjs`
- files ending in `.js` and the `type` field of nearest parent package.json equals `module`

Attention

    es6 module doesn't support json, doesn't guess extensions, doesn't expand directory

### dynamic module
For es6 module, you cannot wrap `import` in a `try catch` like `common.js`.

```js
// following code will get error
console.log('begin dynamic import')
try {
  import { a } from './mod1.js'
} catch (err) {
  console.log('err: ', err)
}
console.log('end dynamic import')
```

You'll get `SyntaxError: Unexpected token` since you need to use import in the top
 level, you must top use it inside other block. And the `console.log` is not executed.

This is because es6 module will first import all modules and then begin to execute.

But is there anyway to achieve dynamic import? Of course yes.

#### using require in es6 module

```js
import { createRequire } from 'module'

console.log('begin dynamic import')
const require = createRequire(import.meta.url)
try {
  require('./mod_require.cjs')
} catch (err) {
  console.log('require ... err', err)
}
console.log('end dynamic import')
```

You can use the `createRequire` of the `module` module to generate a `require` and use
 it to import `common.js` module.

Attention, you need to name the to be required file with `.cjs` suffix. Otherwise node
 will treat it as es6 module and the require will fail.

You can add some `console.log` in the required file and you'll find that its output
 is displayed between the `begin dynamic import` and `end dynamic import`. So that
 we can assure that the `require` is synchoronous.

#### use the `import()` function
You can also use `import()` function to import a es6 module dynamically. It returns a
 promise. You can await it even in top level.

```js
// mod1.js
try {
  const module = await import('./mod2.js')
  // do something with module at here
} catch (err) {
  console.log(err)
}
```

Attention, es6 module will first import modules and then execute code. So that if you
 have following code, be careful about the executing order.

```js
// mod3.js
console.log('begin import')
import * as mod1 from 'mod1.js'
console.log('end import')
```

You'll find that even the `await import(./mod2.js)` in `mod1.js` is executed before the
 `console.log('begin import')`. So remember that modules imported first.

### references
- [node 14: esm api](https://nodejs.org/docs/latest-v14.x/api/esm.html)
- [MDN: import](https://wiki.developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/import)
