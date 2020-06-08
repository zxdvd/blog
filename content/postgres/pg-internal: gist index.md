```metadata
tags: postgres, pg-internal, index, gist
```

## pg-internal: gist index

GiST stands for generalized search tree. It is mainly used for multi-dimensional data
 since it uses [r-tree](../data_structure/rtree introduction.md) internally.

One import benefit is that it supports indexing customized types. Just provide some
 basic functions (compare, union, split and some others) for a data type, then you can
 create GiST index for this data type. No need to write your own index method, no need
 to deal with wallog, concurrency and other annoying things. That's why it is called
 `generalized` search tree.

The official extensions `hstore` and `ltree` have implemented these interfaces so that
 them can use the GiST index.
