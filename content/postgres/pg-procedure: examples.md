<!---
tags: postgres, procedure, plpgsql
-->



## returns only one row
#### return a single value

``` sql
    create function test1() returns int as $$
            select 10;
    $$ language sql immutable;
```
For `language sql`, it seems that result of the last select is returned. e.g., replace body of above function as following
, it will return 10.
``` sql
      select 1,2,3;
      select 10;
```

same function write using plpgsql
``` sql
    create function test1() returns int as $$
    declare result int;
    begin
            select 20 into result;
            return result;
    end;
    $$ language plpgsql immutable;
```

#### returns multiple values via OUT parameter

``` plpgsql
    create function fn1 (a int, b text, OUT out1 int, OUT out2 text) as $$
		begin
			select id, name  into out1, out2 from table1;
		end;
    $$ language plpgsql;

	-- for language sql
    select id, name from table1;

    -- use it like `select * from fn1(1, 'abc');`
 ```

#### returns multiple values via create new type
First create type for the returned values.

``` sql
create type typ_data as (a int, b int);

create or replace function get_data() returns typ_data as $$
    select x, x * 2 from generate_series(1, 5) as x;            -- this will only returns one row
$$ language sql immutable;
```

plpgsql example
``` sql
create type type_test1 as (a int, b int);
create or replace function test1() returns type_test1 as $$
declare result type_test1;
begin
    select 10,20 into result;
    return result;
end $$ language plpgsql;
select test1();
```

#### returns multiple values via table name
Just like this for below `returns multiple rows`, just `returns TABLE_NAME`.

<br>

## returns multiple rows
#### returns multiple rows via `returns table`
need to use `return query` to select the result when  using plpgsql.
```
		create function get_table() returns table (a int, b int) as $$
		begin
		        return query select x, x * 2 from generate_series(1, 5) as x;
		end
		$$ language plpgsql immutable;
```

Just select the result while using sql language.

``` sql
		create function get_table() returns table (a int, b int) as $$
		        select x, x * 2 from generate_series(1, 5) as x;
		$$ language sql immutable;
```

#### attention: be care of `return query select`
Giving following example, you can copy and try it:

```sql
create or replace function test1(int) returns table(s text) as $$
begin
        if ($1 > 10) then
                return query select '>10';
                -- return;
        end if;
        return query select '<=10';
end $$ language plpgsql;
select test1(1);
select test1(100);
```
You may find `select test(1)` works as you expected. But `select test1(100)` returns two rows, `>10` and `<=10`.

Attention here:

    return query select WON'T stop function execution, it just gives rows back.
    If you want to stop after it, you can add another line `return;` after it. (uncomment -- in above function)

	
#### returns multiple rows via `returns setof TABLE_NAME`
For SETOF, you can using existed table name

``` sql
		CREATE FUNCTION get_roles() RETURNS SETOF pg_roles AS $$
			SELECT * FROM pg_roles;
		$$ LANGUAGE sql;
```
		
#### returns multiple rows via `returns setof CREATED_TYPE`
Just like return one row result, created new type first, and then `returns setof xxx_type`.


#### returns multiple rows via `returns setof record`
Benefits of this is that you needn't to define the return types.
However, then you cannot query it like `select * from get_table()` since type is not static.
You can query like `select get_table()`, or `select * from get_table() as t(a int, b int)`

```
		create function get_table() returns setof record as $$
		        select x, x * 2 from generate_series(1, 5) as x;
		$$ language sql immutable;
```
