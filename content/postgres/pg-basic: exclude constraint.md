```metadata
tags: postgres, pg-basic, constraint, exclude, gist
```

## postgres basic: exclude constraint

The unique constraint is very useful when we need to keep one column like username, phone
 or combination of multiple columns like phone and email unique.

But think about the course arranging case:

    we need to arrange a time range like `(starttime, endtime)` of a teacher for different
    students. And we must assure that the time ranges of same teacher shouldn't overlap.

It's very common requirement in time scheduling system. And there are many similar cases
 in GEO related areas, like we want to make circles or boxes have no overlaps.

The answer for all these cases is `exclude constraint`.

You can add `exclude constraint` that uses complex expression on a table and postgres will
 ensure that the result of the expression on each row will exclude with each other.

### a simple example
Following is a simple example with an exclude constraint on the `tstzrange` type.

```sql
create table test_exclude (
    id int,
    period tstzrange,
    exclude using gist (period with &&)
);
insert into test_exclude values (1, '[2020-01-01, 2020-01-03)'), (1, '[2020-01-03, 2020-01-06)');

-- let's insert a time range that overlap with previous inserted data
insert into test_exclude values (1, '[2020-01-04, 2020-01-07)');

-- output
conflicting key value violates exclusion constraint "test_exclude_period_excl"
DETAIL:  Key (period)=(["2020-01-04 00:00:00+08","2020-01-07 00:00:00+08")) conflicts with existing key (period)=(["2020-01-03 00:00:00+08","2020-01-06 00:00:00+08")).
```

The `exclude constraint` make sure that each `tstzrange` excludes with each others, like other
 constraints.

### expression explain
How to understand the `exclude using gist (period with &&)`?

The `exclude constraint` relies on the `gist index` underground, that's why `using gist`.
 The `period` is the to be excluded column while the `&&` is the operator. The `period with &&`
 means that

    You WON'T get two rows satisfied `row1.period && row2.period = true`, then period is excluded
    with each other with operator `&&`.

### more examples
#### unique using gist
So if we use expression like `id with =`, it means that `id` is excluded with each other. Similar
 effect likes the unique constraint (but unique has better performance).

```sql
create table test_exclude1 (
    id int,
    exclude using gist (id with =)
);
```

#### multiple rows exclude constraint
Sometimes we only want timerange excluded within same student or teacher, like following:

```sql
create table test_exclude2 (
    id int,
    student_id int,
    period tstzrange,
    exclude using gist (student_id with =, period with &&)
);
insert into test_exclude2 values (1, 100,'[2020-01-01, 2020-01-03)'), (2,100, '[2020-01-03, 2020-01-06)');
-- it's ok to add overlapped timerange for another student
insert into test_exclude2 values (1, 200,'[2020-01-02, 2020-01-03)'), (2,200, '[2020-01-04, 2020-01-07)');

-- but you cannot add overlapped timerange for a same student
insert into test_exclude2 values (1, 200,'[2020-01-02, 2020-01-03)');
```

From this example, we can find that actually you can add many standalone expressions for an
 excluded constraint. Like

    exclude using gist (col1 with op1, col2 with op2, col3 with op3)

And postgres make sure that there **won't** have any two rows that match

    r1.col1 op1 r2.col1 AND r1.col2 op2 r2.col2 AND r1.col3 op3 r2.col3 = true

#### exclude across groups
Sometimes, we need old versions or snapshots of data. It's common for product management or
 blog posts management. Each time we modified a row, we inserts a new row with higher version
 number.

Let's take blog posts management as example, the table may like following:

```sql
create table blog_post (
    blog_id int,
    version int,
    slug text,
    content text,
    primary key (blog_id, version)
)
```

Each blog post has a `slug` that's the url to access the blog. And of course, two different
 blogs must not have same `slug`. But we cannot simply add a unique constraint on `slug` since
 a blog may have different versions so that many rows (same blog_id) can have same `slug`.

So how to add a constraint to achieve unique across groups (a group is rows with same blog_id)?

Of course, the answer is `exclude constraint`. We can add following constraint (exclude
 constraint can be added later):

```sql
alter table blog_post add constraint
    blog_post_const_slug exclude using gist (slug with  =, blog_id with !=);
```

It assures that there **won't** have two rows that have same `slug` (slug1 = slug2) while
 different `blog_id` (blog_id1 != blog_id2).

```sql
insert into blog_post values (1, 1, 'blog-1', 'first blog');
insert into blog_post values (1, 2, 'blog-1', 'first blog edit');    -- edit blog1
insert into blog_post values (1, 3, 'blog-1-version3', 'first blog edit again');   --edit blog1 again

-- insert a new blog with slug of blog 1, this got error
insert into blog_post values (2, 1, 'blog-1', 'copy first blog');
conflicting key value violates exclusion constraint "blog_post_const_slug"
DETAIL:  Key (slug, blog_id)=(blog-1, 2) conflicts with existing key (slug, blog_id)=(blog-1, 1).
```

### how it works internally
The `exclude constraint` relies on `gist index` which uses `rtree` internally. `rtree` works
 well for multiple dimensions data. It's very easy to check whether an to-be-inserted row will
 overlap with existed rows.

### references
- [pg official doc: exclusion constraint](https://www.postgresql.org/docs/12/ddl-constraints.html#DDL-CONSTRAINTS-EXCLUSION)
- [my blog: rtree](../data_structure/rtree introduction.md)
