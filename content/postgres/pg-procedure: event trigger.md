```metadata
tags: postgres, procedure, trigger
```

## postgres: event trigger

We can use triggers to do some hooks when a row changed. Triggers only work for DML,
 what about DDL? How to add hooks or accouting for DDL like `create table` or `alter table`?

`event trigger` is used for DDL just like trigger for DML.

Currently, postgres supports 4 kinds of events: `ddl_command_start`, `ddl_command_end`,
 `table_rewrite` and `sql_drop`. The `ddl_command_start` and `ddl_command_end` will be
 triggered for all DDL commands normally. I said normally since `ddl_command_end` may
 not triggered if previous stages got failed.

You can check the [matrix](https://www.postgresql.org/docs/12/event-trigger-matrix.html)
 to find what events will a DDL trigger.

### exceptions
Currently, event trigger won't work for DDL that targets at shared objects, thus
 databases, roles and tablespaces.

### simple demo
Let's add a simple event trigger to print all DDLs.

```sql
create function log_ddl() returns event_trigger as $$
begin
    raise info 'ddl: user: %', session_user || ', tag: ' || tg_tag || ', query: ' || current_query();
    insert into test_log_ddl values (now(), tg_tag, current_query());
end $$ language plpgsql;

create event trigger et_log_ddl on ddl_command_start execute procedure log_ddl();
```

Then execute some DDLs to test it.

```sql
postgres@localhost:test> drop table t1;
INFO:  ddl: user: postgres, tag: DROP TABLE, query: drop table t1
DROP TABLE
Time: 0.004s

postgres@localhost:test> create table t1 (id int);
INFO:  ddl: user: postgres, tag: CREATE TABLE, query: create table t1 (id int)
CREATE TABLE
Time: 0.003s

postgres@localhost:test> alter table t1 add name text;
INFO:  ddl: user: postgres, tag: ALTER TABLE, query: alter table t1 add name text
ALTER TABLE
Time: 0.006s

postgres@localhost:test> drop function log_ddl cascade;     -- drop the trigger function itself
INFO:  ddl: user: postgres, tag: DROP FUNCTION, query: drop function log_ddl cascade
NOTICE:  drop cascades to event trigger et_log_ddl
DROP FUNCTION
Time: 0.003s
```

You can filter the DDLs for event triggers using command tags like following

```sql
create event trigger et_log_ddl on ddl_command_start
when tag in ('create table', 'alter table')
execute procedure log_ddl();
```

### event trigger functions
Except functions like `current_query()`, there are some specific functions for event
 triggers only.

#### pg_event_trigger_ddl_commands
This function can only be used in event `ddl_command_end`. You can get some infomation
 about the DDL, like `classid`, `objid`, `objsubid`, `command_tag`, `object_type`,
 `schema_name`, `object_identity`, `in_extension` and `command`. Attention, `command`
 cannot be output directly, you'll get error when trying to `raise info command`.

```sql
create function log_ddl() returns event_trigger as $$
declare
    r record;
begin
raise info 'ddl trigger: %, query: %', tg_tag, current_query();
for r in select *  from pg_event_trigger_ddl_commands() t1 loop
    raise info 'subcommand: %, %, %, query:%', r.command_tag, r.schema_name, r.objid::regclass::text, current_query();
end loop;
end $$ language plpgsql;

create event trigger et_log_ddl on ddl_command_end execute procedure log_ddl();
```

Attention about `pg_event_trigger_ddl_commands()`, a simple command may lead to multiple
 rows since it will use multiple commands internally.

For example, `create table t1(id serial primary key);` will lead to `create sequence`,
 `create table` and `create index`. The DDL trigger only fires once, you can only get
 the 3 subcommands in `pg_event_trigger_ddl_commands`.

```sql
db11=# create table t1 (id serial primary key);
INFO:  ddl trigger: CREATE TABLE, query: create table t1 (id serial primary key);
INFO:  subcommand: CREATE SEQUENCE, public, t1_id_seq, query:create table t1 (id serial primary key);
INFO:  subcommand: CREATE TABLE, public, t1, query:create table t1 (id serial primary key);
INFO:  subcommand: CREATE INDEX, public, t1_pkey, query:create table t1 (id serial primary key);
INFO:  subcommand: ALTER SEQUENCE, public, t1_id_seq, query:create table t1 (id serial primary key);
CREATE TABLE
```

#### pg_event_trigger_dropped_objects
This function can only be used in `sql_drop` event to get information about the dropped
 objects.

### references
- [pg doc: event trigger overview](https://www.postgresql.org/docs/12/event-trigger-definition.html)
- [pg doc: event trigger matrix](https://www.postgresql.org/docs/12/event-trigger-matrix.html)
- [pg doc: event trigger functions](https://www.postgresql.org/docs/12/functions-event-triggers.html)
