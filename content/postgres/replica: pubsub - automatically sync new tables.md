

## replica: automatically sync new tables for logical replication

Attention: this article is not applicable at now due to
[this bug](https://www.postgresql.org/message-id/flat/CAMa1XUh7ZVnBzORqjJKYOv4_pDSDUCvELRbkF0VtW7pvDW9rZw%40mail.gmail.com).

We can easily setup logical replication via
 [publication and subscription](./replica: publication and subscription.md).
 However, it has some limitations.

- You cannot publish tables using wildcard like `crm_*`
- For partitioned tables, you need to use `alter publication` to add new partitions
- You need to create table in replication
- You need to `refresh subscription` each time you add new table

But we can make all these automatically using trigger and event trigger.

### short steps
- add event trigger for `create table`, so that we can add targeted tables to our publication.
- sync DDLs to replication to create tables automatically
- add event trigger at replication side to trig `refresh subscription` accordingly

### detailed steps
Create ddl log table and publication first.

```sql
create table test_log_ddl (created_at timestamptz, cmd text, ddl text);
create publication pub_test1 for table test_log_ddl;
```

Create the event trigger to log DDLs and alter publication accordingly. I use the `sync_%`
 to filter tables that to be synced, you can change this to fit your situations.

```sql
create function log_ddl() returns event_trigger as $$
declare
    r record;
begin
    raise info 'ddl: user: %', session_user || ', tag: ' || tg_tag || ', query: ' || current_query();
    insert into test_log_ddl values (now(), tg_tag, current_query());
    for r in select * from pg_event_trigger_ddl_commands()
        where objid::regclass::text like 'sync_%' and command_tag in ('CREATE TABLE')
    loop
        raise info 'ddl_command: cmd: %s, objid: %s', r.command_tag, r.objid::regclass::text;
        execute format('alter publication pub_test1 add table %s', r.objid::regclass::text);
    end loop;
end $$ language plpgsql;

create event trigger et_log_ddl on ddl_command_end execute procedure log_ddl();
```

At the replication side, we create the ddl log table and setup subscription to receive changes.

```sql
create table test_log_ddl (created_at timestamptz, cmd text, ddl text);

create subscription sub_test1
    connection 'host=localhost port=5433 user=postgres dbname=db11'
    publication pub_test1;
```

Then add row trigger to execute the synced DDLs automatically. I expected this to be
 work. However, it will corrupt your database due to
 [bug](https://www.postgresql.org/message-id/flat/CAMa1XUh7ZVnBzORqjJKYOv4_pDSDUCvELRbkF0VtW7pvDW9rZw%40mail.gmail.com).

So we cannot do this last step before the bug is fixed.

```sql
create function replay_log_ddl() returns trigger as $$
begin
    raise info 'will replay ddl tag: %, query: %', new.cmd, new.ddl;
    if new.cmd = any(ARRAY['CREATE TABLE', 'ALTER TABLE']) then
        execute(new.ddl);
    end if;
    if new.cmd = any(ARRAY['CREATE TABLE']) then
        execute('alter subscription sub_test1 refresh publication');
    end if;
    return new;
end $$ language plpgsql;

create trigger t_replay_ddl after insert on test_log_ddl for each row execute procedure replay_log_ddl();

-- enable trigger on replication
alter table test_log_ddl enable always trigger t_replay_ddl;
```
