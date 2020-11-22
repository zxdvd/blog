```metadata
tags: postgres, procedure, trigger
```

### temporary disable trigger
Sometimes you may want to disable triggers temporarily, e.g., when bulk inserting or 
update whole table to do some fix.

You can disable the trigger and then do the action and then enable it.

    alter table t1 disable trigger trigger_name;
    -- your insert or update
    alter table t1 enable trigger trigger_name;

Of course, you can wrap them in a transaction.

Or you can change a session config:

    set session_replication_role = replica;

This is intended to be used in replication cluster to disable triggers. But you can 
also use it in your session to disable triggers.

To restore to origin setting, set it to `default` or `origin`.

