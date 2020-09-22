```metadata
tags: web, frontend, react, react-hooks
```
## react hooks: useEffect
The `useEffect` hook is used to achieve side effect when the dependent state or props
 changed. It likes the `componentDidMount` and `componentDidUpdate` of the class style.

A example from official docs:

```js
import React, { useState, useEffect } from 'react';

function Example() {
  const [count, setCount] = useState(0);

  // Similar to componentDidMount and componentDidUpdate
  useEffect(() => {
    // Update the document title using the browser API
    document.title = `You clicked ${count} times`;
  });
  return (
    <div>
      <p>You clicked {count} times</p>
      <button onClick={() => setCount(count + 1)}>Click me</button>
    </div>
  );
}
```

### basic usage
You may see following useEffect code styles:

```js
    useEffect(() => { doSomething })
    useEffect(() => { doSomething }, [name, age, props.name])
    useEffect(() => { doSomething }, [])
```

It's easy to understand the second one, the callback only executed when dependencies
 change.

By default (the first one), the useEffect callback runs after every render, like
 combination of componentDidMount (first render) and componentDidUpdate (every later
 render).

We use useEffect to create side effects but sometimes we may not want to trigger side
 effects after every render. We may want to do conditional side effects.

Then we can add dependencies of this side effect in an array just like the second one
 so that it will only be triggered when dependencies changed.

The third one is a special case of the second one. It means the side effect depends
 on nothing. So that it will be triggered only once after the first render.

### side effect cleanup
You can return a function in callback of useEffect to do some cleanup.

```js
  useEffect(() => {
    ChatAPI.subscribeToFriendStatus(props.friend.id, handleStatusChange);
    return () => {
      ChatAPI.unsubscribeFromFriendStatus(props.friend.id, handleStatusChange);
    };
  });
```

It likes the `componentDidUnmount` but there is a big difference. The `componentDidUnmount`
 runs only once after component unmounted but cleanup function of useEffect runs after
 every render.

This looks wierd at first, but it's a good design to avoid bugs. Just like above example,
 you need to subscribe new friend status each time friend id changed. But many may only
 unsubscribe the last one when component unmounted.

```
    subscribe friend id 1
                                        friend id change to 2
    subscribe friend id 2
                                        friend id change to 3
    subscribe friend id 3
                                        component unmount
    unsubscribe friend id 3
```

To avoid this bug, you need to do unsubscribe in `componentDidUpdate` to unsubscribe
 previous friend id.

The cleanup of useEffect avoids this by doing cleanup of previous effects before applying
 next effects.

### effect dependencies and comparison
When you use useEffect with dependencies, react will check the dependencies array with
 previous one's. Do how does it check this? Actually, react use `Object.is()` for each
 element of the array.

This may lead following problems when elements are not primitive types.

#### unnecessary side effects
Just like following example, you may pass a generated object to child component and child
 component depends on this object. Since the object is generated on the fly, it is a new
 object in each render so that `Object.is()` always got false and then the callback of
 useEffect calls each time the parent rerendered.

```js
    function Student({ student }) {
        useEffect(() => { doSomething }, [student])
        // render
    }

    function Class({ students }) {
        return (<div>
            { students.map(s => <Student student={{ id: s.id, name: s.name }} />) }
        </div>)
    }
```

This mostly affects performance and rarely leads to bug. To avoid this, try to use data of
 primitive types in dependency array. And only includes the real dependency like `id`.

    useEffect(() => { doSomething }, [student.id, student.name])     // suggested
    useEffect(() => { doSomething }, [student])     // not suggested

#### effect not triggered as expected
It's ok to have some unnecessary side effects but sometimes the following style may get
 unexpected bug, the call may not be triggered.

    useEffect(() => { doSomething }, [student])

If parent component keeps passing props that have same references, child component cannot
 aware of changes since it uses `Object.is()` to do comparison. Then you may find that
 child component didn't update while parent changed.

So you need to be very careful when using object in dependencies. You may want a deep
 comparing or customized comparing.

#### deep comparing or customized comparing
There isn't official hook for this but we can write one. Design an api as following:

    useCustomizeEffect(() => { doSomething }, depArray, isChanged)
    function isChanged(prevDeps, currentDeps) { doCustomizedCompare }

We can implement it by using a `prevousValue` hook to memory prevous dependencies. And
 then compares it with current dependencies and decides whether to increment the counter.
 (usePreviousValue is not official hook, you can write one or find a package).

```js
    function useCustomizeEffect(callback, dependencies, isChanged) {
        const prevDependencies = usePreviousValue(dependencies)
        // let's use a counter value as dependency of useEffect
        counterRef = useRef()
        // change the counter only when customized comparing shows that something changed
        if (isChanged(prevDependencies, dependencies)) {
            counterRef.current = (counterRef.current || 0) + 1
        }
        useEffect(callback, [counterRef.current])
    }
```


### references
- [react official: hooks effect](https://reactjs.org/docs/hooks-effect.html)
- [react official: hooks references/useEffect](https://reactjs.org/docs/hooks-reference.html#useeffect)

