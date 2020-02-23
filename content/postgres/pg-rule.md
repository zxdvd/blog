<!---
tags: postgres, rule
-->

## pg rule

Rule is a machanic that you can use it to change the `sql` in **rewrite** stage.

### syntax
You can define a rule that match any of `SELECT, INSERT, UPDATE, DELETE` and rewrite it 
to do nothing, or execute additional commands or replace it with other commands.

```sql
CREATE [ OR REPLACE ] RULE name AS ON [ SELECT | INSERT | UPDATE | DELETE ]
    TO table_name [ WHERE condition ]
    DO [ ALSO | INSTEAD ] { NOTHING | command | ( command ; command ... ) }
```

### a use case to convert all hard delete to soft delete

```sql
create table t1 (
    id int,
    name text
);
create table t2 (
    id serial primary key,
    name text,
    deleted_at timestamptz
);
create rule rule1 as on delete to t1
    do instead update t2 set deleted_at = now() where id = old.id;

create or replace rule rule2 as on update to t1
    do instead update t2 set name = new.name where id = new.id;

create rule rule3 as on insert to t1
    do instead insert into t2 values (new.id, new.name);

create rule "_RETURN" as on select to t1
    do instead select id,name from t2 where deleted_at is null;

insert into t1 values (1, 'ab'), (2, 'cd'), (3, 'ef'), (4, 'hij');
select * from t1;
select * from t2;
delete from t1 where id = 1 or name = 'cd';
```

The table t1 is like a updatable view of table t2. But all delete to t1 are converted
 to soft update on t2. And the deleted rows are not visible in t1.

#### rule on select
Creating `on select` rule on table will be converted to a `create view`. And you can
 check it in `pg_class`, the relkind of t1 in `pg_class` is 'v' which means view.

And there is a lot of restrictions on creating `on select` rule.

- rule name must be `_RETURN`
- cannot create on partitioned table or partition
- cannot create on not empty table
- cannot create on table that has index or trigger

So it is suggested to using view instead of creating select rule.

#### rule on delete and on update and on insert
For insert, update and delete, you can use New.XX or OLD.XX in the action. The action
 seems only supporting insert one each time. But actually you can insert multiple values.
Same to update and delete.
The rule on delete and on update, in above example, I use `where id=old.id` but it 
supports update or delete of any kind of where.

But why?

For example, if you execute the sql `delete from t1 where id = 1 or name = 'cd';`, postgres
will rewrite it as

    delete from t2 where id in (select id from t1 where id = 1 or name = 'cd');

We can get it from result of explain:

```sql
create rule rule1 as on delete to t1
    do instead update t2 set deleted_at = now() where name = old.name;

postgres@localhost:test1> explain (verbose) delete from t1 where name = 'cd';
+--------------------------------------------------------------------------------------+
| QUERY PLAN                                                                           |
|--------------------------------------------------------------------------------------|
| Update on public.t2  (cost=0.15..32.31 rows=1 width=56)                              |
|   ->  Nested Loop  (cost=0.15..32.31 rows=1 width=56)                                |
|         Output: t2.id, t2.name, now(), t2.ctid, t2_1.ctid                            |
|         Inner Unique: true                                                           |
|         ->  Seq Scan on public.t2 t2_1  (cost=0.00..24.12 rows=1 width=10)           |
|               Output: t2_1.ctid, t2_1.id                                             |
|               Filter: ((t2_1.deleted_at IS NULL) AND (t2_1.name = 'cd'::text))       |
|         ->  Index Scan using t2_pkey on public.t2  (cost=0.15..8.17 rows=1 width=42) |
|               Output: t2.id, t2.name, t2.ctid                                        |
|               Index Cond: (t2.id = t2_1.id)                                          |
+--------------------------------------------------------------------------------------+
```

If I changed the delete rule to use `name = old.name`, the explain also changed:

```sql
postgres@localhost:test1> explain (verbose) delete from t1 where id = 1 or  name = 'cd';
+---------------------------------------------------------------------------------------------------------+
| QUERY PLAN                                                                                              |
|---------------------------------------------------------------------------------------------------------|
| Update on public.t2  (cost=26.96..52.58 rows=6 width=56)                                                |
|   ->  Hash Join  (cost=26.96..52.58 rows=6 width=56)                                                    |
|         Output: t2.id, t2.name, now(), t2.ctid, t2_1.ctid                                               |
|         Hash Cond: (t2.name = t2_1.name)                                                                |
|         ->  Seq Scan on public.t2  (cost=0.00..21.30 rows=1130 width=42)                                |
|               Output: t2.id, t2.name, t2.ctid                                                           |
|         ->  Hash  (cost=26.95..26.95 rows=1 width=38)                                                   |
|               Output: t2_1.ctid, t2_1.name                                                              |
|               ->  Seq Scan on public.t2 t2_1  (cost=0.00..26.95 rows=1 width=38)                        |
|                     Output: t2_1.ctid, t2_1.name                                                        |
|                     Filter: ((t2_1.deleted_at IS NULL) AND ((t2_1.id = 1) OR (t2_1.name = 'cd'::text))) |
+---------------------------------------------------------------------------------------------------------+
```

Then always that 

    rule works at the rewrite phase, it just rewrite input sql to new sql.

And it may lead to weird behavior. Try following example, it uses rule to log update of table.

```sql
create table t1 (
    id serial primary key,
    name text
);
create table t1_log (
    id serial primary key,
    t1_id int,
    name text
);

create rule rule1 as on insert to t1
    do also insert into t1_log(t1_id,name) values (new.id, new.name);

insert into t1 (name) values ('ab'), ('cd');
insert into t1 (name) values ('ef'), ('hi');
select * from t1;
select * from t1_log;

-------------- RESULT  ------------------
postgres@localhost:test1> select * from t1;
+------+--------+
| id   | name   |
|------+--------|
| 1    | ab     |
| 2    | cd     |
| 5    | ef     |
| 6    | hi     |
+------+--------+
postgres@localhost:test1> select * from t1_log;
+------+---------+--------+
| id   | t1_id   | name   |
|------+---------+--------|
| 1    | 3       | ab     |
| 2    | 4       | cd     |
| 3    | 7       | ef     |
| 4    | 8       | hi     |
+------+---------+--------+
```

The result is really surprised. The `t1_id` in t1_log doesn't match id of t1.
And sequence of table t1 is disturbed.

But we can guess and explain the result. The id of t1 is replaced with
 `nextval(t1_id_seq)` and the `new.id` is also replace with it. This can
 explain that why t1_id of t1_log is continued from id of t1.

We got this because `rule works at rewrite phase that before the execute phase`.
Then all `new.XXX` will be expanded. If they are sequence, you'll get `nextval()`.
If you use other functions in your sql, they'll be expanded too.

If the function has side effect or not immutable, you'll get unexpected result.

So to archieve the log result, trigger is better option than rule since trigger
 works at execute phase then all `new.XXX` is evaluated.

### references
[official doc: create rule](https://www.postgresql.org/docs/current/sql-createrule.html)
