```metadata
tags: postgres, procedure, trigger
```

## postgres: trigger

Trigger, the name is really self explanation. You can define triggers for command like
 `INSERT`, `UPDATE`, `DELETE` and `TRUNCATE`.

You can define trigger to be fired `BEFORE`, `AFTER` or `INSTEAD OF` the origin event.
And you can choose to trig `FOR EACH ROW` or `FOR EACH STATEMENT`.

Attention

- The `INSTEAD OF` can only be used on views and must be defined as `FOR EACH ROW`.

### use trigger to set updated_at to now()
A simple demo to set `updated_at` to now() using trigger.

```sql
create table test_updated_at (id int, updated_at timestamptz);
create function fn_test_updated_at() returns trigger as $$
begin
    new.updated_at = now();
    return new;
end $$ language plpgsql;

create trigger tg_test_updated_at before insert or update
on test_updated_at for each row execute procedure fn_test_updated_at();
```

The `updated_at` will be set to `now()` for any insert or update even you set it explicitly.

### use trigger to log changes of important table
Suppose we have a `account` table and we want to log all history changes of it.

```sql
create table account (id serial primary key, name text, phone text, created_at timestamptz);
create table account_history (id serial primary key, account_id int, name text, phone text, created_at timestamptz);

create function fn_account_log() returns trigger as $$
begin
    raise info 'trigger fn_account_log, id: %', new.id;
    insert into account_history (account_id, name, phone, created_at) values (old.id, old.name, old.phone, now());
    return new;
end $$ language plpgsql;
create trigger tg_account after update on account for each row execute procedure fn_account_log();
```

Then each time you update one or many rows of table `account`, the trigger will log
 each old rows in table `account_history`.

If you only want to log when some important columns (for example, phone) were changed, you
 and handle it in procedure or add a `when` condition for the trigger. Like following:

```sql
create trigger tg_account after update
on account for each row
when (new.phone is distinct from old.phone)     -- add when condition
execute procedure fn_account_log();
```

Then an update without changing `phone` won't trig the trigger.

### who and which process executes the trigger
Sometimes you may need to use `current_user` in the trigger. And the search_path matters
 when you use objects in trigger.

So it's very important to understand which user executes the trigger? Is it a special
 builtin user, the trigger creator or the user who trigged the trigger?

And is the trigger executed in a standalone process or current process? What about
 setting?

We can get the answer from following test:

```sql
drop function if exists p1_t1 cascade;
drop table if exists t1;

create table t1 (id int);

create function p1_t1() returns trigger as $$
declare
    r record;
begin
    raise info 'in trigger current_user: %, session_user: %', current_user, session_user;
    select pg_backend_pid() as x into r;
    raise info 'in trigger pid: %', r.x;
    select setting into r from pg_settings where name = 'search_path';
    raise info 'in trigger search path: % ', r.setting;
    return old;
end $$ language plpgsql;
create trigger tg_t1 after insert on t1 for each row execute procedure p1_t1();

select pg_backend_pid(), session_user, current_user;
insert into t1 values (1);

---- output
+------------------+----------------+----------------+
| pg_backend_pid   | session_user   | current_user   |
|------------------+----------------+----------------|
| 15232            | u1             | u1             |
+------------------+----------------+----------------+
SELECT 1
INFO:  in trigger current_user: u1, session_user: u1
INFO:  in trigger pid: 15232
INFO:  in trigger search path: public, d
```

I tested this with different users and found that the trigger is execute in same session
 as the sql that trigged the trigger using same user. Just like the session user executes
 the trigger procedure.

### trigger fire settings: origin, replica, local
By default, triggers won't execute when replication role is `replica` (use `show session_replication_role;`
 to get it) since replications are often physical block to block sync or logical row to row sync.

But you can still force enable triggers on replication using following sql:

    alter table TABLE_NAME enable always trigger TRIGGER_NAME;

### temporary disable trigger
Sometimes you may want to disable triggers temporarily, e.g., when bulk inserting or update
 whole table to do some fix.

You can disable the trigger and then do the action and then enable it.

    alter table t1 disable trigger trigger_name;
    -- your insert or update
    alter table t1 enable trigger trigger_name;

Of course, you can wrap them in a transaction.

Or you can change a session config:

    set session_replication_role = replica;

According to above section, `replica` won't execute trigger by default.

To restore to origin setting, set it to `default` or `origin`.

### references
- [pg doc: create trigger](https://www.postgresql.org/docs/12/sql-createtrigger.html)
