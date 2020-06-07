<!---
tags: js, node, prototype
-->

## Javascript prototype

Javascript is prototype based OOP language. You can create a new object based on prototype
 of another object. And prototypes can be chained. And when you access property of object,
 it will follow the prototype chains if not found.

```js
const A = function(){}
const a = new A();        // you may think a is a instance of A
a.__proto__ === A.prototype       // result is true
a.__proto__.__proto__ === Object.prototype // true since A's prototype is Object
```

Each object has a property `__proto__`, it points to its prototype. In above example, `__proto__`
 of `a` points to `prototype` of A. Attention, it is not pointing to `A` but `A.prototype`.

```js
console.log(a.abc)  // undefined
A.abc = 100
A.prototype.abc = 200
console.log(a.abc)  // 200
```

If a property is not defined, it will follow the `__proto__` recursively to lookup it.
In above example, `a.abc` is got via `A.prototype.abc` but not `A.abc`.

A image from [stackoverflow](https://stackoverflow.com/questions/9959727/proto-vs-prototype-in-javascript):

![javascript prototype chain](./images/prototype-chain.png)

### object property and prototype property
When you `new` an instance, it can access properties of its prototypes. But it cannot
 access properties of the constructor. They are different. Following is an example:

```js
function Person() {}
Person.Name1 = function () { return 'James' }
Person.prototype.Name2 = function () { return 'James' }
console.log(Person.Name1())
console.log(Person.Name2())               // TypeError Person.Name2 is not a function

const p1 = new Person()
console.log(p1.Name1())                   // TypeError p1.Name1 is not a function
console.log(p1.Name2())
```


### prototype and constructor
When you `new` an object. The constructor of its prototype is called. Actually, the
 constructor is the defined function itself.

```js
function Person(name, age) {
    this.name = name
    this.age = age
}
const p1 = new Person('James', 18)
Person.prototype.constructor === Person     // true
p1.__proto__.constructor === Person         // true
p1.constructor === Person            // true, the instance also has a constructor property
```

### Object.create vs `new`
They both create object but the `Object.create` won't call the constructor of the prototype
 while the `new` will.

The `__proto__` property of the created objects  are different. The `__proto__` of `Object.create`
 is not pointed to the prototype as `new`.

```js
function A() {}
b = Object.create(A)
b.__proto__ === A.prototype          // false
b.__proto__ === A                    // true
```

### prototype and inheritance
A example from [MDN: object model](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Details_of_the_Object_Model).

```js
function Manager() {
  Employee.call(this);
  this.reports = [];
}
Manager.prototype = Object.create(Employee.prototype);
Manager.prototype.constructor = Manager;
```


### isPrototypeOf and instanceof

Attention, `obj instanceof cls` checks against prototype of cls but not cls itself.

A demo code of instanceof implementation from MDN:

```js
function instanceOf(object, constructor) {
   object = object.__proto__;
   while (object != null) {
      if (object == constructor.prototype)
         return true;
      if (typeof object == 'xml') {
        return constructor.prototype == XML.prototype;
      }
      object = object.__proto__;
   }
   return false;
}
```

You can only use `obj instanceof cls` when cls is a callable function. Otherwise you'll
 get `TypeError: Right-hand side of 'instanceof' is not callable`. While `cls.isPrototypeOf(obj)`
 avoids this.

```js
const a = {}
const b = Object.create(a)
console.log(a.isPrototypeOf(b))      // got false, but not error
console.log(b instanceof a)        // got TypeError
```

### references
- [MDN: details of the object model](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Guide/Details_of_the_Object_Model)
