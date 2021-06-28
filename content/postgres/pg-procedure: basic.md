```metadata
tags: postgres, procedure, plpgsql
```

## postgres procedure: basic

### declare variables
For plpgsql, you can use variables but you need to declare it at first. And it doesn't
 support declare variable with pseudo types like `anyelement`, `anyarray`. You'll get
 compiling error.

### special variables
There is a special variable `FOUND` that you can use it to check result of previous
 statement. It is true if any of `SELECT INTO`, `PERORM`, `UPDATE/INSERT/DELETE`,
 `FETCH`, `MOVE`, `FOR/FOREACH`, `RETURN QUERY` or `RETURN QUERY EXECUTE` returns
 or modifies or deletes at least one row. Otherwise it is false (0 rows returned
 or affected).

With help of `FOUND`, you can write code like

```sql
    select id into sid from t1 where a > 100 limit 1;
    if not FOUND then
        -- deal with not found
    end if;
```

There are some special variables for trigger only:

    NEW, OLD, TG_NAME, TG_WHEN, TG_LEVEL (row or statement), TG_OP, TG_RELID
    TG_TABLE_NAME, TG_TABLE_SCHEMA, TG_NARGS, TG_ARGV

### exception
You can use `exception when` to capture exceptions like following:

```sql
DO $$
begin
    select 1/0;     -- trig a divided by 0
    exception
    when division_by_zero then
        raise info 'get a exception divided by 0';
    when others then
        raise info 'get another exception';
end $$ language plpgsql;
```

You can get a list of internal errors and exceptions at
 [here](https://www.postgresql.org/docs/10/errcodes-appendix.html).

### dynamic sql executing
Sometimes, you may want to generate dynamic sql and execute it. Then you can use the
 `EXECUTE` keyword to execute a formated sql. For example:

```sql
DO $$
declare r record;
begin
for r in select * from pg_class where relkind in ('r', 'm') limit 3
loop
    EXECUTE format('select ''%s'', pg_relation_size(%s)', r.relname, r.oid);
end loop;
end $$ language plpgsql;
```

The `EXECUTE` can return results as table so that you can use `FOR IN` to loop the
 result. Let's change above example a little like following to show the result of
 dynamic executed sql:

```sql
for r in select * from pg_class where relkind in ('r', 'm') limit 3
loop
    for r1 in execute format('select ''%s'', pg_relation_size(%s)', r.relname, r.oid)
    loop
        raise info 'result %', r1;
    end loop;
end loop;
```

### security definer
By default, function will be executed using current user (SECURITY INVOKER). However,
 you can set it to be executed using the owner via option `SECURITY DEFINER`.

You can define following function using user A and execute `select tmp1()` using user
 B and you'll find that it returns user A as current_user.

```sql
create or replace function tmp1 () returns name as
    ' select current_user ' language sql SECURITY DEFINER;
```

`SECURITY DEFINER` is similar to the `suid` bit of linux file. Sometimes normal
 user may need superuser power to do something but you don't want to lift the user
 to superuser. Then you can wrap the needed features in a `SECURITY DEFINER` function
 and grant this function to him/her.

### parameters

#### variadic paramters
You can define function with variadic paramters using `VARIADIC ARRAY`, like following:

```sql
create or replace function tmp1 (t1 text, variadic arr integer[]) returns void as $$
begin
    raise info 'parameters: %, %', t1, arr;
end $$ language plpgsql;

select tmp1('hello', 1), tmp1('hello', 1, 2, 3);
```

You can use it as normal array in function body.

### loop statement
A simple `LOOP` statement:

```sql
    BEGIN
        LOOP
            EXIT WHEN counter = 100;            -- add break condition
            counter := counter + 1;
        END LOOP;
    END;
```

We can also use `FOR` loop statement:

```sql
    FOR counter IN 1..100         -- for counter in 1..100 by 1 (1 and 100 inclusive);
    LOOP
        -- so something here
    END LOOP;
```

You can use `FOR IN` to loop an `anyarray` array:

```sql
create or replace function test_loop_array(arr1 anyarray) returns void as $$
declare
    idx anyelement;
begin
    FOR idx IN 1..array_length(arr1, 1)
    LOOP
        RAISE INFO 'idx: %, element: %', idx, arr1[idx];
    END LOOP;
end $$ language plpgsql;

select test_loop_array(ARRAY[20,30,40]);
select test_loop_array(ARRAY['a', 'b', 'c']);
```

It's easy to loop a query result via `FOR x IN QUERY`:

```sql
    FOR record IN select * from pg_class limit 10
    LOOP
        RAISE INFO 'row: %', record.relname;
    END LOOP;
```

Sometimes you may want to loop an dynamic sql. Then you can use the `FOR x IN EXECUTE sql`:

```sql
    FOR record IN EXECUTE concat_ws(' ', 'select * from', 'pg_class', 'limit 10')
    LOOP
        RAISE INFO 'row: %', record.relname;
    END LOOP;
```

The `FOREACH x IN ARRAY` is used to loop an array:

```sql
    FOREACH el IN ARRAY array(select relname from pg_class limit 10)
    LOOP
        RAISE INFO 'row: %', el;
    END LOOP;
```

You cannot use `FOREACH` to loop and array of type `anyarray` since you cannot declare
 `el` as `anyelement`. You'll get compiling error like
  `variable "el" has pseudo-type anyelement`.

### references
- [book: postgres server programming second edition](http://sd.blackball.lv/library/PostgreSQL_Server_Programming_Second_Edition_(2015).pdf)
- [pg doc: FOUND variable](https://www.postgresql.org/docs/10/plpgsql-statements.html#PLPGSQL-STATEMENTS-DIAGNOSTICS)
- [pg doc: errcodes and exceptions](https://www.postgresql.org/docs/10/errcodes-appendix.html)

