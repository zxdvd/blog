```metadata
tags: js, node, eslint
```

## basic: eslint custom rules and plugin

Sometimes, we may need to add some customer lint rules to our project to check
 some project specific conventions. We can write custom rules to achieve this.

`eslint` will parse source code to AST and check rules according to the AST.
In your customized rule, you can define function to check with each AST node.

Following is a simple example that check with the `Literal` node.

```js
module.exports = {
  create(context) {
    return {
      Literal(node) {
        if (node._babelType === 'JSXText' && isInsideJSX(node) && !isWhiteSpace(node.value)) {
          // ignore jsx attr like className="cls"
          if (node && node.parent && node.parent.type === 'JSXAttribute') {
            return
          }
          context.report({
            node,
            message: `you need to use the i18n func, "${node.value}"`,
          })
        }
      },
    }
  },
}
```

You can check [estree](https://github.com/estree/estree/blob/master/es5.md) to find
 all node types of es5 and other ECMA specs.

You can also define function to check against a selector like `IfStatement > BlockStatement`.

And you can learn from existed matured project like
 [react eslint rules](https://github.com/yannickcr/eslint-plugin-react/tree/master/lib/rules).

### plugin
You can bundle multiple rules into a plugin easily:

```js
module.exports = {
    rules: {
        r1: require('./r1.js'),
        r2: require('./r2.js'),
        r3: require('./r3.js'),
    }
}
```

Suppose your plugin package is `eslint-plugin-myplugin`, then you can use rules
 like `myplugin/r1`.

### load custom plugin
`eslint` supports load custom plugins from command line using `eslint --rulesdir PLUGIN_PATH`.
However, there is not a similar option in `.eslint.rc` config file.  It's really annoying
 since I have to do project specific lint command. Before this, I simply config my IDE (vim)
 to run `eslint` with current file. It works for all projects since it will search the `.eslint.rc`
 automatically.

You can use package like `eslint-plugin-local-rules` to avid this. But I'd prefer the
 method that overwrite the module resolve function from
 [eslint issue 2715](https://github.com/eslint/eslint/issues/2715):

```js
// .eslintrc.js

const Module = require('module')

const oldResolver = Module._resolveFilename

function newResolver(request, requester, ...rest) {
  if (request === 'eslint-plugin-local') {
    return require.resolve('./script/eslint_rules/index.js')
  } else {
    return oldResolver.apply(this, [request, requester, ...rest])
  }
}

Module._resolveFilename = newResolver

module.exports = {
  plugins: [ 'local' ],
  rules: {
    'local/i18n': 'error',
    'no-undef': 'off',
  },
}
```

### references
- [eslint official: working with rules](https://eslint.org/docs/developer-guide/working-with-rules)
- [eslint official: rule selector](https://eslint.org/docs/developer-guide/selectors)

