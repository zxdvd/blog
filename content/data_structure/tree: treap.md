```metadata
tags: data-structure, tree, bst, treap
```

## tree: treap
Treap is a binary search tree that each node has a priority property. Before explaining
 about the purpose of it, let's think about the caveats of BST at first.

### BST worst case
The insert, search and delete of BST is `O(logN)`, but it's the average case. In the
 worst case, for example, insert **sorted** list into an empty BST, the BST degenerates
 to a linked list. And then the complexity becomes `O(N)`. Really bad.

### priority of node
Treap is an improved BST that can overcome this, and also keep the code simple. It adds
 a priority for each node and requires each node to satisfy that:

    priority of parent must be large or equal than priority of all children.

You can change `larger or equal than` to `less or equal than`, that's the same.

But, what's the purpose of the priority?

For treap, the priority is generated randomly when inserting a new node. To satisfy the
 priority requirement, you need to rotate the node.

By this way, the tree depth is controlled by sequence of generated random numbers
 but not sequence of input value.

The input sequence may be biased or even ordered which leads to worse case BST. But
 we can treat the distribution of random numbers as uniform. And the depth of uniformly
 random BST is `O(logN)`.

### implementation
I found two implementations of treap at source code of go.

- [runtime/mgclarge.go](https://github.com/golang/go/blob/go1.12.6/src/runtime/mgclarge.go)
- [runtime/sema.go](https://github.com/golang/go/blob/go1.12.6/src/runtime/sema.go)

