<!---
tags: postgres, query, update
-->

## postgres upsert
There are multiple ways to achieve the upsert or insert ignore effects.

* use `insert on conflict` (postgres 9.5 and above)
* use `try insert catch exception do update` in function
* do a atomic `update; insert if no row updated` in application level

### insert on conflict
From postgres 9.5, the INSERT supports `on conflict [conflict_target] conflict_action` 
clause. You can use it to achieve the **upsert** or **insert or ignore** effects.

``` sql
insert into t_upsert values (1, 10,'abc'), (2, 12, 'cc'), (3, 13, 'dd')
on conflict on constraint t_upsert_uid_key
-- on conflict (id)
do update set name = excluded.name
returning *;
```
Like above example, you can still insert multiple values. You can add `on conflict (id)`
 or `on conflict on onstraint t_upsert_uid_key` clause and a `do update set name = excluded.name`
  action.
Then if the value can be inserted without conflict, it is inserted.
If there is conflict and the conflict matched, the action is executed.

Attention:
- You cannot use multiple `on conflict` in same insert sql.
- The insert will fail if you specify one constraint but it violates another constraint.
- You can still use `returning *` if you want

#### upsert multiple values
You can insert multiple values just like above example. And the result could be some inserted 
and some updated.

However, there shouldn't have conflict between these values that will be inserted. Following 
inserting will get error.

``` sql
insert into t_upsert values (3, 13,'abc'), (3, 13, 'dd')
on conflict (id)
do update set name = excluded.name;
------------- RESULT ---------------
ON CONFLICT DO UPDATE command cannot affect row a second time
HINT:  Ensure that no rows proposed for insertion within the same command have duplicate constrained values.
```

#### insert on conflict do nothing
To achieve the insert ignore effect, just replace `do update` with the `do nothing` action.

### insert catch exception and then update
You can also write a function that try insert first and do update in exception. This will 
work for postgres version below 9.5.

Following is the function that sequelize (a node.js ORM) used:

``` sql
CREATE OR REPLACE FUNCTION pg_temp.sequelize_upsert(OUT created boolean, OUT primary_key text)  AS $$
BEGIN
    insert into t_upsert values (3, 13, 'abc');
    created := true;
EXCEPTION WHEN unique_violation
THEN
    update t_upsert set name = 'abc' where uid = 13;
    created := false;
END; $$ LANGUAGE plpgsql;
SELECT * FROM pg_temp.sequelize_upsert();
```

### references
[official doc: sql-insert](https://www.postgresql.org/docs/current/sql-insert.html)
