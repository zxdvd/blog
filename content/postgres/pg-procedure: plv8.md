<!---
tags: postgres, procedure, plv8
-->

plv8 is a **trusted language** extension for postgres. Official document at [here](https://plv8.github.io/).

For debian or ubuntu, you can install it simply using `apt-get  install postgresql-10-plv8` (change version 
according your pg version).  After install it, you need to `create extension plv8` for each database you need 
to use it.  Then you can using following query to check version of it

```
DO $$ plv8.elog(NOTICE, plv8.version); $$ LANGUAGE plv8;
```

Currently plv8 do not support ES6. So you cannot use arrow function, template string and other features.

#### a simple plv8 demo

```
create or replace function test(a int, b int) returns setof json as $$
	var sql = 'select x from generate_series($1::int, $2::int) x'
	var result = plv8.execute(sql, [a, b])
	result.forEach(function(row) {
		row.y = row.x * 2
	})
	return result
$$ LANGUAGE plv8;

select * from test(1, 4);
```

Actually, you can returns almost all types like plpgsql procedure.

#### error: field name / property name mismatch
Your returned object or array of object should have all columns of the type you want to return.
Example:

```
create type type_test as (a int, b text);

create or replace function test() returns type_test as $$
	var result = {a: 1}
	return result
$$ LANGUAGE plv8;

select * from test();    -- you'll get error `field name / property name mismatch`
```

You can return null for missed fields like
```
    return {a:1, b: null}
```


#### error: input of anonymous composite types is not implemented
You cannot define a function that `returns record` and then call it like `select * from fn() as (a int, b int)`.
plv8 do not support this.

