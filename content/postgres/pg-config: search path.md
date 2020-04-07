<!---
tags: database, postgres, config
-->

## postgres: search path of schema

In postgres, you can have multiple schemas under each database. Schema is just namespace
 of tables. You can join tables from different schemas. It's like database of mysql.

You may not aware the schema since there is a default schema: `public`. You needn't
 to specify the schema name when using table or creating table. The default `public`
 is used, **if you didn't change the default search_path config**.

### what is search_path
The `search_path` is similar to the `PATH` environment of linux system. The `PATH`
 decides location and order when looking up a command. While the `search_path` decides
 the order of schema names when looking up a table.

By default, the search_path is `"$user", public`. The `"$user"` means current user. So
 if you have a schema that matches with user name exactly, then this schema will be
 used. Otherwise, the `public` schema will be used.

You can use sql `show search_path;` to get search_path config of current session.

### wrong schema and not granted schema
Schema that is not existed or the user hasn't been granted `USAGE` on it will be ignored.

Suppose that you have table with name `t1` in schema B, C, D and you don't have permission
 to access schema B. Then if your search_path is `A, B, C, D`, `select * from t1` will
 match `t1` in C. To use `t1` of D, you need to use `D.t1` emplicitly.

### set search_path
You can change search_path of each connection session using `set search_path=a,b,c,d;`.
This will work for all following queries of this session only.

If you want to change search_path of a user permanently, you can set it as a role option.

```sql
alter role u1 in database db1 set search_path = a, b, public, c, d;

-- following will change search_path of all roles
alter role ALL in database db1 set search_path = a, b, public, c, d;

-- following will change search_path of a role for every database
alter role u1 set search_path = a, b, public;

-- you can reset a setting to default
alter role postgres in database warehouse reset search_path ;
```

Attention:

- this will not take effect for current session
- should not use `set search_path = 'a,b,c';` or `set search_path="a,b,c";`, you won't
  get error, but here `'a,b,c'` is a single schema name not three schema names.

### References:
- [official doc: schema search path](https://www.postgresql.org/docs/11/ddl-schemas.html#DDL-SCHEMAS-PATH)
- [official doc: runtime config](https://www.postgresql.org/docs/current/runtime-config-client.html)
