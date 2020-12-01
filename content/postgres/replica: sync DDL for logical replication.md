```metadata
tags: postgres, procedure, replication, trigger
```

## replica: sync DDL for logical replication

From postgres 10, it supports logical replication using publication and subscription.
However, it doesn't suport replication of DDLs. If you add a new column on master,
 the replication will get errors soon since it tries to insert data with un-existed
 column.

So how to do logical replication with DDLs synced?

We can achieve it via [event trigger](./pg-procedure: event trigger.md) and
 [row trigger](./pg-procedure: trigger.md).

### explain shorted
- prerequisite: you need to set `wal_level=logical` to use logical replication
- create a table to store all to be executed DDLs
- store DDLs in the table via event trigger
- replicate the table to slaves
- replay the replicated DDLs on slaves via row trigger

### detailed steps
Let's create a simple to table to log all DDLs:

```sql
create table test_log_ddl (created_at timestamptz, cmd text, ddl text);
```

Add event trigger to store future DDLs into this table:

```sql
create function log_ddl() returns event_trigger as $$
begin
    raise info 'ddl: user: %', session_user || ', tag: ' || tg_tag || ', query: ' || current_query();
    insert into test_log_ddl values (now(), tg_tag, current_query());
end $$ language plpgsql;

create event trigger et_log_ddl on ddl_command_start execute procedure log_ddl();
```

Setup publication to publish the `test_log_ddl` table on the master side:

```sql
create publication pub_test1 for table test_log_ddl;
```

Setup table and subscription on slave side:

```sql
create table test_log_ddl (created_at timestamptz, cmd text, ddl text); -- you need to create the same table

create subscription sub_test1
    connection 'host=localhost port=15433 user=postgres dbname=postgres'
    publication pub_test1;
```

Let's test the replication. Create a test table and select the `test_log_ddl` table
 you'll find the create table sql in it.

Now let's add row trigger to replay the DDLs.

```sql
create function replay_log_ddl() returns trigger as $$
begin
    raise info 'will replay ddl: %', new.ddl;
    execute(new.ddl);
    return new;
end $$ language plpgsql;

create trigger t_replay_ddl after insert on test_log_ddl for each row execute procedure replay_log_ddl();
```

Let's create another table to test. However, the sql has synced to slave but table is
 not created. Anything wrong?

Actually, trigger are disabled on replicated tables (both physical and logical). We need to enable it.

```sql
alter table test_log_ddl enable always trigger t_replay_ddl;
```

And then test again and the DDL replays successfully.

### filter DDLs
Sometimes you may want to sync part of DDLs, you can create event trigger with tag filter
 like following (log only `create table` and `alter table`):

```sql
create event trigger et_log_ddl on ddl_command_start
when tag in ('create table', 'alter table')
execute procedure log_ddl();
```

You can add complex filter logic about schema name, table name in event trigger function
 using `pg_event_trigger_ddl_commands`. Like following:

```sql
create function log_ddl() returns event_trigger as $$
declare
    r record;
begin
    raise info 'ddl: user: %', session_user || ', tag: ' || tg_tag || ', query: ' || current_query();
    for r in select * from pg_event_trigger_ddl_commands() t1
        where objid::regclass::text like 'sync_%'
    loop
        insert into test_log_ddl values (now(), tg_tag, current_query());
    end loop;
end $$ language plpgsql;

-- pg_event_trigger_ddl_commands can only be used in ddl_command_end
create event trigger et_log_ddl on ddl_command_end execute procedure log_ddl();
```

### multiple DDL statements
If you execute multiple DDLs in one sql like `create table t1(id int); create table t2(id int)`,
 it will be execute one by one, and the event trigger will be triggered twice but not
 once. The `current_query()` will return two splitted DDLs.
