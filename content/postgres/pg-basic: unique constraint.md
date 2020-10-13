```metadata
tags: postgres, pg-basic, constraint, unique
```

## pg-basic: unique constraint
Unique constraint ensures that data in a column or multiple columns is unique among
 whole table or some rows (filter by a where).

Unique constraint is achieved by creating a unique index.

### simple example
The `username` column of a `users` table is used to be unique. And we can create it
 like following:

```sql
create table users (id serial, username text, created_at timestamptz);
create unique index idx_users_username on users (username);
insert into users (username, created_at) values ('zhang', now()), ('li', now());
```

You'll get error when trying to insert a row with same username:

```sql
test1> insert into users (username, created_at) values ('zhang', now());
duplicate key value violates unique constraint "idx_users_username"
DETAIL:  Key (username)=(zhang) already exists.
```

We should always prefer unique constraint to application level uniqueness like `findOrCreate`.
Application code may have bugs while the unique constraint is strong and safe.

### NULLs
A common mistake about unique constraint is about NULL values. Since NULL is not comparable.
You'll get null for `select null = null`, `select null != null`, `select 1 = null`, `select 1 != null`.
So two NULLs are not equal. Then you can insert multiple rows even there is unique constraint.

```sql
local> insert into users (username, created_at) values (null, now()), (null, now());
INSERT 0 2
Time: 0.005s
local> select * from users;
+------+------------+-------------------------------+
| id   | username   | created_at                    |
|------+------------+-------------------------------|
| 1    | zhang      | 2020-10-13 19:04:02.538412+08 |
| 2    | li         | 2020-10-13 19:04:02.538412+08 |
| 5    | <null>     | 2020-10-13 19:13:20.748114+08 |
| 6    | <null>     | 2020-10-13 19:13:20.748114+08 |
+------+------------+-------------------------------+
```

So if you don't like this, you need to add `NOT NULL` constraint to the `username` column.


And this is a difference between primary key and unique constraint. Primary key must be
 `NOT NULL` while an unique column can be NULL.

#### unique NULL
What about you really want unique NULL? We can achieve this via partial index and expression.
We can convert NULL to other comparable values like 0,1,false,true and add unique constraint
 on the converting expression.

```sql
local> delete from users where username is null;
local> create unique index idx_users_username_1 on users ((true)) where username is null;

local> insert into users (username, created_at) values (null, now());
INSERT 0 1

local> insert into users (username, created_at) values (null, now());
duplicate key value violates unique constraint "idx_users_username_1"
DETAIL:  Key ((true))=(t) already exists.
```

Attention, it is `on users ((true))` not `on users (true)`.

### unique across multiple columns
You can also create unique constraint on multiple columns. Suppose you are a SAAS provider
 and username only needs to be unique in a same customer. Then you can add a `customer_id`
 column and set unique on `customer_id` and `username`.

```sql
create table users (id serial, customer_id int, username text, created_at timestamptz);
create unique index idx_users_username on users (username, customer_id);
insert into users (customer_id, username, created_at) values (1, 'zhang', now());
```

If you insert `(1, 'zhang', now())` again, you'll get unique constraint error.

Be careful about NULLs, not only `(null, null, now())` can be inserted multiple times but
 also `(1, null, now())` can be inserted multiple times.

Actually, if any one of the composed columns is null, then the unique constraint won't
 work on it and you can insert it multiple times without error. You'd better add `NOT NULL`
 for all of the composed columns.

You can use following partial index to achieve uniqueness with NULLs.

```sql
local> create unique index idx_users_username_1 on users (coalesce(username, ''), coalesce(customer_id, 0))
    where username is null or customer_id is null;
```

### partial unique
Sometimes, we may need to implement the soft delete, thus add a `deleted_at` or `deleted_by`
 column to mark the row as deleted instead of deleting the row from table.

And we only need unique constraint with undeleted rows. We can use partial index to achieve
 this.

```sql
create unique index idx_users_username on users (username) where deleted_at is not null;
```

By this way, the uniqueness is only applied to undeleted rows.


### common errors

#### duplicate key value violates unique constraint "pg_type_typname_nsp_index"
You may get it when doing `create table if not exists xxx` concurrently. You can
 reproduce it by doing this in two separated transactions.

Postgres will insert a type whose name is same as the table when creating new
 table. The `pg_type_typname_nsp_index` is a unique index of table `pg_type` on
 `(typname, typnamespace)`.  The `create table if not exists` is not atomic internally,
 so you'll get this error when trying to create two table with same name.

