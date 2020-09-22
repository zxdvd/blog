```metadata
tags: js, node, module, require
```

## node.js: the require hook
I used to be asked about the internal implementation of `require('@babel/register')`.
What did it do so that you can use unsupported syntax such as `import xx from yy` and
 polyfills.

I've thought of two kinds of implementation:

- monkey patch the `require` function
- maybe there has some internal hooks to use so that we can transform syntax tree on the fly

### monkey patch require
You can replace the origin `require` function like following

```js
const mod = require('module')

const origin_require = mod.prototype.require

mod.prototype.require = function () {
  console.log('require: ', arguments)
  // do something here
}
```

### require hook
The `@babel/register` uses this method. It uses [pirates](https://github.com/ariporad/pirates)
 to add hooks.

Actually, there is no official hooks to use. The `pirates` library also does some monkey
 patch. But it target is the internal `_compile` function of each file extension.

You can check the node.js source file to get the detail of the require function. The
 `Module.prototype.require` calls `Module._load` which mainly deals with module cache and
 then call `Module.prototype.load(filename)`. The `load` function will call loader according
 the file extension.

Currently, there is three builtin loaders stored at `Module._extension`. Then are `.js`,
 `.json` and `.node` (native binary module).

Following is source code of the js loader. It reads the whole file and then calls the
 `Module.prototype._compile = function(content, filename)`.

```js
// Native extension for .js
Module._extensions['.js'] = function(module, filename) {
  if (filename.endsWith('.js')) {
    const pkg = readPackageScope(filename);
    // Function require shouldn't be used in ES modules.
    if (pkg && pkg.data && pkg.data.type === 'module') {
      const parent = moduleParentCache.get(module);
      const parentPath = parent && parent.filename;
      const packageJsonPath = path.resolve(pkg.path, 'package.json');
      throw new ERR_REQUIRE_ESM(filename, parentPath, packageJsonPath);
    }
  }
  const content = fs.readFileSync(filename, 'utf8');
  module._compile(content, filename);
};
```

The `pirates` chooses to monkey patch the `.js` loader and the `_compile` method. It
 gets the content and applies the hook to change the content and then calls the origin
 `_compile`.

Following is the core part of `pirates` to add a hook.

```js
function addHook(hook) {
    const oldLoader = Module._extensions['.js']
    Module._extensions['.js'] = function(mod, filename) {
        const oldCompile = mod._compile
        mod._compile = function (code) {
            const newCode = hook(code, filename)      // applies the hook
            return oldCompile(newCode, filename)      // calls the origin _compile
        }

        oldLoader(mod, filename)                      // calls the origin loader
    }
}
```

The babel runtime is something that loads the label config and chooses configured
 transforms properly and applies these transforms as hooks so that code will be
 transformed before loading.

### references
- [github: pirates - hijack require](https://github.com/ariporad/pirates)
- [node source: cjs module loader](https://github.com/nodejs/node/blob/master/lib/internal/modules/cjs/loader.js#L1098)
- [node 10 official doc: require resolve filepath](https://nodejs.org/dist/latest-v10.x/docs/api/modules.html#modules_all_together)

