```metadata
tags: sql, cheatsheet, postgres, mysql, sqlserver
```

## sql cheatsheet

#### show server version

    postgres/mysql       $ select version()
    mysql/sqlserver      $ select @@version

#### limit N

    postgres/mysql       $ select * from table_xxx limit 10
    sqlserver            $ select top 10 from table_xxx

#### drop table

    postgres/mysql/sqlserver>=2016    $ drop table [if exists] table_xxx [cascade]
    sqlserver                         $ if object_id('table_xxx', 'U') is not null drop table table_xxx

#### prepare statement and binding

Postgres referres parameters by position, like `$1`, `$2`, `$3`.

    PREPARE fooplan (int, text, bool, numeric) AS INSERT INTO foo VALUES($1, $2, $3, $4);
    EXECUTE fooplan(1, 'Hunter Valley', 't', 200.00);

In sql server, you can refer parameter by name or position, like `@name`, `@id`, `@p1`.

    DECLARE @P1 INT;
    EXEC sp_prepare @P1 OUTPUT,
        N'@P1 NVARCHAR(128), @P2 NVARCHAR(100)',
        N'SELECT database_id, name FROM sys.databases WHERE name=@P1 AND state_desc = @P2';
    EXEC sp_execute @P1, N'tempdb', N'ONLINE';
    EXEC sp_unprepare @P1;


    postgres/mysql       $ select version()

#### base64
postgres:

      select encode('hello', 'base64');    -- output aGVsbG8=
      select convert_from(decode('aGVsbG8=', 'base64'), 'utf8');  -- decode returns bytea

mysql:

      select to_base64('hello');    -- output aGVsbG8=
      select from_base64('aGVsbG8=');

#### string related
- concat multiple strings

      postgres$ select 'hello' || ' world' || '!';
      postgres$ select 'hello' || null;         -- you got NULL if concat with null
      mysql   $ select concat('hello', 'world', '!');

- remove leading and trailing space

      postgres$ -- you can also trim any other characters, and you can choose trim leading, trailing or both
      postgres$ select '#' || trim(both ' *=' from ' **  b.=c = ')  || '#';   -- get `#b.=c#`
      mysql$ select trim('   abc   ');


#### timezone related
- show timezone

    postgres$ show timezone;
    postgres$ select current_setting('timezone');
    mysql   $ select @@session.time_zone;

- get timezone offset

    postgres$ select extract(timezone from now())/3600;

#### datetime related
- interval to seconds

    postgres$ select extract(epoch from interval '1 hours');

- timestamp to seconds

    postgres$ select extract(epoch from now());
    mysql   $ select to_seconds(now());
