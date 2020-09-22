```metadata
tags: postgres, pg type, array
```

## pg type: array


### check array length
We can use `array_length(anyarray, dimension)` to get length of array. But null and empty
 array are very special, both of them return 0.

```sql
local> select array_length(null::int[], 1) as null_len, array_length('{}'::int[], 1) as empty_len;
+------------+-------------+
| null_len   | empty_len   |
|------------+-------------|
| <null>     | <null>      |
+------------+-------------+
```

We can also use `cardinality(anyarray)` in this situation, and `cardinality` can separate
 null array and empty array. It returns null for null and 0 for empty array.

```sql
local> select cardinality(null::int[]) as null_len, cardinality('{}'::int[]) as empty_len;
+------------+-------------+
| null_len   | empty_len   |
|------------+-------------|
| <null>     | 0           |
+------------+-------------+
```

So use `array_length` if you want to filter both null array and empty array, and use
 `cardinality` if you only want null or empty array.
