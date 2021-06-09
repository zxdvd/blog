```metadata
tags: postgres, sequence
```

## postgres sequence

Postgres `sequence` is a kind of internal object that used to achieve the auto increment
 feature.

The most popular use case is defining a column type as `serial` like following following:

```sql
create table if not exists t1 (
    id serial,
    name text,
    primary key (id)
);

postgres@localhost:test1> \d t1
+----------+---------+--------------------------------------------------+
| Column   | Type    | Modifiers                                        |
|----------+---------+--------------------------------------------------|
| id       | integer |  not null default nextval('t1_id_seq'::regclass) |
| name     | text    |                                                  |
+----------+---------+--------------------------------------------------+
Indexes:
    "t1_pkey" PRIMARY KEY, btree (id)
```

If will create a sequence automatically and set the default value of the column as
 `nextval(SEQUENCE)`. Each time `nextval` is called, it will advance to next value
 and return it.

You can create sequence mannually and set it as default value of a column. However,
 by this way, it won't drop the sequence when you drop the table.

```sql
create sequence seq_t2;
create table if not exists t2 (
    id int default nextval('seq_t2'::regclass),
    name text
);
```

By default, the increment step is 1. So you'll get 1,2,3,4,5... But you can create
 a sequence like `create sequence seq_test1 increment by 10 start 100;` so that it
 will start from 100 and increment by 10 each time.


### duplicate key value violates unique constraint
A common error about sequence is the `duplicate key value violates unique constraint`.
Mysql also has similar problem too.

Take above table `t1` as example:

```sql
postgres@localhost:test1> insert into t1(id,name) values (1, 'a'), (2, 'b'), (3, 'c');
INSERT 0 3
Time: 0.004s
postgres@localhost:test1> insert into t1 (name) values ('d'), ('e');
duplicate key value violates unique constraint "t1_pkey"
DETAIL:  Key (id)=(1) already exists.
```

If you insert rows with explicit `id`, then the default `nextval` of sequence is not
 called and next value of it is still 1. Then next time you insert a row without id
 specified, it will get a default value 1, this violates the unique constraint.

To avoid this problem, you need to update next value of sequence using `setval()`,
 for above example, you can update it using following sql:

```sql
postgres@localhost:test1> select setval('t1_id_seq', (select max(id) from t1))
+----------+
| setval   |
|----------|
| 3        |
+----------+
postgres@localhost:test1> insert into t1 (name) values ('d'), ('e');
INSERT 0 2
```

For mannually created table or renamed table, the sequence may not be `tablename_id_seq`,
 then you can use function `pg_get_serial_sequence(tablename, colname)` to get sequence.
 So you can also use following sql to update sequence, actually it's more accurate.

    select setval(pg_get_serial_sequence('t1', 'id'), max(id)) from t1;
