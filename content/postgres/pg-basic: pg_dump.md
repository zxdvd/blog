```metadata
tags: postgres, dump
```

## postgres basic: pg_dump

### dump roles and tablespaces
Sometimes, you may want to dump only roles to another server. You can get it via
 `pg_dumpall`. In postgres, roles and tablespaces related informations are stored
 as global objects while others like tables, schemas, functions are restricted in
 a specific database.

To dump global objects, you need to use tool `pg_dumpall`.

Dump roles and tablespaces (password and role configurations included)

    $ pg_dumpall -h localhost -U postgres -v --globals-only > db-globals.sql

Dump roles only:

    $ pg_dumpall -h localhost -U postgres -v --roles-only > db-roles.sql

### dump all functions, procedures
How about restore functions and procedures to another database or server? You
 can use the `--schema-only` option of pg_dump or pg_dumpall. The output contains
 table definitions, types, functions, procedures and triggers.

    $ pg_dump -h localhost -p 15433 -U postgres --schema-only -f schemas.sql DB_NAME

If you want to exclude table definitions, you can write script to filter out them.
I also found a solution at postgres mailing list,
 [Export ALL plpgsql functions/triggers](https://www.postgresql.org/message-id/D960CB61B694CF459DCFB4B0128514C203937F9B%40exadv11.host.magwien.gv.at)

    1. dump schemas using **custom** format (-Fc)
        $ pg_dump -U postgres --schema-only -Fc -f schemas.pg_dump DB_NAME

    2. list object names of the custom format dump file using pg_restore and filter using awk
        $ pg_restore -l schemas.pg_dump | awk '/[0-9]*; [0-9]* [0-9]* (FUNCTION|TRIGGER)/ { print }' > schemas.list

    3. load contents of the fitered objects using the list
        $ pg_restore -L schemas.list -f schemas.sql schemas.pg_dump

You can do complex filter using above method.

Attention: function or procedure may depends on table or other type, restoring may get
 failed if you exclude the type definitions.
