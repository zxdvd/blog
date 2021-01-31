```metadata
tags: postgres, pg-basic, cte, with
```

## postgres basic: recursive query
The `with CTE` of postgres has a strong feature that it supports recursive query:
 the query can refer itself in subquery to do recursive query.

A simple example:

```sql
with recursive t as (
    select 1 as i
    union all
    select i+1 from t
)
select * from t limit 10;
-- the output is ten rows from 1 to 10.
```

The subquery of the CTE refers `t` itself. It uses `previous t` in the subquery to
 get `next t`, recursively.

A recursive CTE is composed following:

    WITH RECURSIVE t as (
        the_initial_query
        UNION  |  UNION ALL
        the_iterate_query
    )

The intial query is a simple query and it must not refer to the with CTE itself. It is
 returned at first and it also feeds as first input of the iterating query.

The iterating query then uses result of **previous** query as with CTE itself and generates
 new result. The new result will be the WITH CTE itself in the next iteration.

But when will the iterating stop? It will stop when the iterating query return 0 rows.
 So you need to be careful and not write query that will iterate forever.

You can use query like `select * from t limit 10` in first example. Though the with CTE
 will iterate forever since it always returns one row in each iterating step, but the
 `limit 10` means 10 rows are enough and then it will stop.

But the better way is add condition to avoid forever loop like following. The iterating query
 will return 0 rows **when i >= 10** and then it will stop.

```sql
with recursive t as (
    select 1 as i
    union all
    select i+1 from t where i < 10
)
select * from t;
```

### use cases
Except for generating sequence like above, is there any other useful use cases?

It's common to define table like following and the `child_ids` means subordinates.

```sql
create table employee (
    id int,
    name text,
    child_ids int[],
    primary key (id)
);
insert into employee values (1, 'boss', '{2,3}'),
    (2, 'leader1', '{4,5}'), (3, 'leader2', '{6,7,8}'),
    (4, 'emp1', null), (5, 'emp1', null), (6, 'emp1', null), (7, 'emp1', null), (8, 'emp1', null);
```

Then how to find all subordinates (direct and indirect) of an employee? Of course,
 you can write application code like:

```js
async function getAllSubordinates(employeeId) {
    const subordinates = []
    const employee = await models.Employee.findById(employeeId)
    let childIds = employee.child_ids
    while (childIds && childIds.length) {
        const subs = await models.Employee.findAll({
            where: { id: childIds },
        })
        subordinates.push(...subs)

        childIds = _.flatten(subs.map(sub => sub.child_ids))
    }
    return subordinates
}
```

However, for this code, it needs to make multiple sql queries to get all the results. The
 deeper the hierarchy is, the more queries it need to do.

With recursive query of postgres, you can get the result with only one sql query:

```sql
with recursive subordinates as (
    select * from employee where id = ${employeeId}
    union all
    select employee.* from employee
        inner join subordinates on employee.id = any (subordinates.child_ids)
)
select * from subordinates;
```

Above query will include current employee in the result. If you don't like this, you can
 filter out it in application code, or change the initial query to
 `select * from employee where id = any (select child_ids from employee where id = ${employeeId})`

The with recursive query is very useful for the tree-like data structure. You can query
 all children and descendants or parents and ancestors in one query.

Attention: take care to avoid forever loop. Make sure that no circular relations in
 your data. And you'd better add `limit N` as a double check.

### references
- [pg official doc: with query](https://www.postgresql.org/docs/12/queries-with.html)
