```metadata
tags: postgres, pg-basic, olap, crosstab, pivot
```

## pg-basic: crosstab function (oracle pivot)
Sometimes you may want to convert rows to columns. It is happened at analysis database
 commonly. Oracle has the `pivot` function to do this. With postgres, you can achieve
 this using `crosstab` function.

Let's create a demo table and fill with data.

```sql
create table student_score (id serial, name text, subject text, score int);
insert into student_score (name, subject, score) values
    ('Zhao', 'Math', 80),
    ('Zhao', 'Sci', 90),
    ('Qian', 'Math', 88),
    ('Sun', 'Math', 91),
    -- ('Sun', 'Phy', 97),
    ('Sun', 'Sci', 92);

local> select * from student_score;
+------+--------+-----------+---------+
| id   | name   | subject   | score   |
|------+--------+-----------+---------|
| 1    | Zhao   | Math      | 80      |
| 2    | Zhao   | Sci       | 90      |
| 3    | Qian   | Math      | 88      |
| 4    | Sun    | Math      | 91      |
| 5    | Sun    | Sci       | 92      |
+------+--------+-----------+---------+
```

We will convert it to following style:

```
+--------+-----------+---------+
| name   | math      | sci     |
+--------+-----------+---------|
| Zhao   | 80        | 90      |
| Qian   | 88        | NULL    |
| Sun    | 91        | 92      |
+--------+-----------+---------+
```

The `crosstab` function belongs to `tablefunc` extension. We need to install it first.

```sql
local> create extension tablefunc;
```

Then we can use `crosstab` to convert rows to columns. The `crosstab` has two variants.
A simple one is like following:

```sql
local> select * from crosstab('select name,subject, score from student_score order by 1,2')
    as t1(name text,math int, sci int)
+--------+--------+--------+
| name   | math   | sci    |
|--------+--------+--------|
| Qian   | 88     | <null> |
| Sun    | 91     | 92     |
| Zhao   | 80     | 90     |
+--------+--------+--------+
```

However, this variant is not suggested since it has serious problem when some groups have
 missing category.

Let's insert a new row with a new category:

```sql
insert into student_score (name, subject, score) values ('Sun', 'Phy', 97);
```

Let's use `crosstab` again, this time we have 3 categories.

```sql
local> select * from crosstab('select name,subject, score from student_score order by 1,2')
    as t1(name text,math int, phy int, sci int);
+--------+--------+--------+--------+
| name   | math   | phy    | sci    |
|--------+--------+--------+--------|
| Qian   | 88     | <null> | <null> |
| Sun    | 91     | 97     | 92     |
| Zhao   | 80     | 90     | <null> |
+--------+--------+--------+--------+
```

We inserted a new category for student Sun and the other two students don't have this category.
The results for student Qian and Sun are right. Postgres gives NULL for missing rows.

But the result for `Zhao` is WRONG, or unexpected. Zhao has a `Sci` score but no `phy` score.
However, postgres just fills values of new category columns from left to right and fills NULL
 if not enough. So if a group has missing rows in the middle, you'll get WRONG result.

We can avoid this by using `crosstab(text source_sql, text category_sql)` as following:

```sql
local> select * from crosstab('select name,subject, score from student_score order by 1',
    'select subject from student_score group by subject order by 1')
    as t1(name text,math int, phy int, sci int);
+--------+--------+--------+--------+
| name   | math   | phy    | sci    |
|--------+--------+--------+--------|
| Qian   | 88     | <null> | <null> |
| Sun    | 91     | 97     | 92     |
| Zhao   | 80     | <null> | 90     |
+--------+--------+--------+--------+
```

Now we got correct result. The `category_sql` is used to get the categories you want to
 convert to columns. You should add `order by XXX` here otherwise you don't know the columns
 order.

And also make sure the category count returned by `category_sql` matches columns you defined.
Otherwise you'll get error like `Query-specified return tuple has 3 columns but crosstab returns 4`.


### references
- [oracle doc: pivot](https://www.oracle.com/technical-resources/articles/database/sql-11g-pivot.html)
- [postgres doc: crosstab](https://www.postgresql.org/docs/11/tablefunc.html)
