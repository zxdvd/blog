<!---
tags: database, postgres, srf
date: 2020-01-12
-->

One reason I like postgres so much is that it has so much useful functions. I think many 
developers may have used functions like `unnest, json_each, generate_series`. These functions 
are so called SRF (Set Returning Functions). They may return multiple rows for each row just 
like one to many JOIN.

A simple example:

```sql
test> select 'hello' as a, generate_series(1,3) b
+-------+-----+
| a     | b   |
|-------+-----|
| hello | 1   |
| hello | 2   |
| hello | 3   |
+-------+-----+
```

However, what if the input parameter has NULL? Let's try it:

```sql
test> select 'hello' as a, generate_series(1,null) b;
+-----+-----+
| a   | b   |
|-----+-----|
+-----+-----+
SELECT 0
test> select 1, unnest(null::int[]);
+------------+----------+
| ?column?   | unnest   |
|------------+----------|
+------------+----------+
SELECT 0
```

The result is empty, 0 rows. I think you may expect it to be `NULL` instead of no rows, right?
I used to expect that and we've got a production bug on this. The default result is like 
`INNER JOIN`. So how to achieve the `LEFT JOIN` effect?

```sql
test> with t1(n) as (values (1), (2), (null)) select *, generate_series(n, 3) b from t1
+-----+-----+
| n   | b   |
|-----+-----|
| 1   | 1   |
| 1   | 2   |
| 1   | 3   |
| 2   | 2   |
| 2   | 3   |
+-----+-----+
SELECT 5

test> with t1(n) as (values (1), (2), (null)) select * from t1 left join generate_series(n, 3) b on true;
+--------+--------+
| n      | b      |
|--------+--------|
| 1      | 1      |
| 1      | 2      |
| 1      | 3      |
| 2      | 2      |
| 2      | 3      |
| <null> | <null> |
+--------+--------+
```

On above example, the `t1` as 3 rows with column `n` has values `[1,2,null]`. The first query does
not return the row with `n=NULL`. For the second query, we use a `LEFT JOIN` so that all rows of t1 
are returned while the column b is NULL when related SRF returns 0 rows.

So if we expect `t1` is always returned just like `LEFT JOIN`, we can always use the 
`LEFT JOIN SRF_FUNCTION ON TRUE`.

Let's dig it a little deeper. Is `NULL` in paramters really matters? The answer is maybe.

When you defined a function, you can specify multiple qualifiers. If you added `STRICT` ( or 
`RETURNS NULL ON NULL INPUT`, they are same), then it means that this function will return NULL 
if any of its paramters are NULL. And for SRF function, it seems that it well return 0 rows.

You can get this info from `pg_proc` using following query:

```sql
-- proisstrict indicates the `STRICT` is set, proretset indicates it is a SRF function
select proname, pronamespace::regnamespace, proisstrict from pg_proc where  proretset;
```

Then what about function without `STRICT`?

Let's write a simple SRF function without `STRICT` and query it with NULL paramter:

```sql
begin;
create function srf1(a int) returns setof int as $$
begin
    return next 1;
    return next 2;
    return next 3;
end $$ language plpgsql;

select srf1(null);
rollback;
```

We can find that it returns 3 rows as expected. And after add `STRICT` it returns 0 rows.

Let's try another one:

```sql
begin;
create function srf1(a int) returns setof int as $$
begin
    return;
end $$ language plpgsql;

select srf1(null) as test_null;
select srf1(1) as test_1;
rollback;
```

This function returns 0 rows no matter about the paramters as this is what it wants to.

Summary:
* If SRF function defined with `STRICT`, it will return 0 rows if any of its paramters are NULL.
* If the executed result of the SRF function is 0 rows, it of course will returns 0 rows even no `STRICT`.
* If you want a `LEFT JOIN` effect, you'd better use it like `LEFT JOIN srf_func(a,b) ON TRUE`.
