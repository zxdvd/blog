```metadata
tags: postgres, replication
```

## postgres replication: publication and subscription

From postgres 10, it supports logical replication using publish and subscribe model.
 Logical replication replicates row level logical changes while physical replication
 replicates on disk file changes byte by byte.

One main benefits of logical replication is that you can filter changes. You can only
 subscribe tables you are interested in. You cannot do this with physical replication.

And you can choose to publish some of `insert, update, delete, truncate`.

### prerequisite
You need to set `wal_level=logical` to enable logical replication. This will increase
 wallog size since logical decoding needs more infomation. Server restart is needed
 to make it work.

### create publication
You can create publication for tables like following:

```sql
create publication pub_test1 for table t1,t2;

t1_u1@localhost:db1> select * from pg_publication
+-----------+------------+----------------+-------------+-------------+-------------+
| pubname   | pubowner   | puballtables   | pubinsert   | pubupdate   | pubdelete   |
|-----------+------------+----------------+-------------+-------------+-------------|
| pub_test1 | 16385      | False          | True        | True        | True        |
+-----------+------------+----------------+-------------+-------------+-------------+

t1_u1@localhost:db1> select * from pg_publication_tables;  -- view based on pg_publication_rel
+-----------+--------------+-------------+
| pubname   | schemaname   | tablename   |
|-----------+--------------+-------------|
| pub_test1 | public       | t1          |
| pub_test1 | public       | t2          |
+-----------+--------------+-------------+
```

#### replica identity
For table with no primary key, you won't get any error when create publication. However,
 you'll get error while issuing `UPDATE` or `DELETE` command.

It hints that you need to set `replica identity` of table. But I suggest to always add
 primary key for table.

### create subscription
Then let's create subscription from another server. Like following:

```sql
create subscription sub_test1
    connection 'host=localhost port=5433 user=postgres dbname=db1'
    publication pub_test1;
```

Attention:

- You'll get hung when creating subscription that connects to a same postgres server.
You need to [set parameter `create_slot = false` and then create slot later](https://www.postgresql.org/docs/12/sql-createsubscription.html).

- You need to be superuser to create subscription, thus different from publication.

- You need to prepare all tables of the publication. Postgres won't check columns at the
 creation but you'll get error while the worker begins to sync.

You can list subscriptions and tables along with status like following:

```sql
db2=# select * from pg_subscription;
 subdbid |  subname  | subowner | subenabled |                    subconninfo                    | subslotname | subsynccommit | subpublications
---------+-----------+----------+------------+---------------------------------------------------+-------------+---------------+-----------------
   16385 | sub_test1 |       10 | t          | host=localhost port=5433 user=postgres dbname=db1 | sub_test1   | off           | {pub_test1}

db2=# select * from pg_subscription_rel ;
 srsubid | srrelid | srsubstate | srsublsn
---------+---------+------------+-----------
   16394 |   16395 | r          | 0/173C310
   16394 |   16398 | r          | 0/173C348
```

### check stat
You can check stat of the replication at master side using view `pg_stat_replication`.

```sql
db1=# select pid,usesysid,usename,backend_start,state,sent_lsn,write_lsn,flush_lsn
             ,replay_lsn,write_lag,flush_lag,replay_lag,sync_priority,sync_state
      from pg_stat_replication ;
  pid  | usesysid | usename  |         backend_start         |   state   | sent_lsn  | write_lsn | flush_lsn | replay_lsn | write_lag | flush_lag | replay_lag | sync_priority | sync_state
-------+----------+----------+-------------------------------+-----------+-----------+-----------+-----------+------------+-----------+-----------+------------+---------------+------------
 19900 |       10 | postgres | 2020-11-30 18:40:36.365958+08 | streaming | 0/174EEC0 | 0/174EEC0 | 0/174EEC0 | 0/174EEC0  |           |           |            |             0 | async

db2=# select * from pg_stat_subscription ;                  -- replication side
 subid |  subname  |  pid  | relid | received_lsn |      last_msg_send_time       |     last_msg_receipt_time     | latest_end_lsn |        latest_end_time
-------+-----------+-------+-------+--------------+-------------------------------+-------------------------------+----------------+-------------------------------
 16394 | sub_test1 | 19899 |       | 0/174EEC0    | 2020-11-30 18:40:47.735425+08 | 2020-11-30 18:40:47.735549+08 | 0/174EEC0      | 2020-11-30 18:40:47.735425+08
```

### add tables to existed publication
You can adjust tables of the publication later using command `alter publication PUB_NAME add/set/drop table TABLE_NAME`.

Let's add another table to an existed publication:

```sql
alter publication pub_test1 add table t3;
```

You need to create the same table at replication side. It won't start automatically.
 You need to `refresh` the related publication to active it.

```sql
alter subscription sub_test1 refresh publication;
```

Above sql will fetch missing tables from master side and start to sync newly added tables.


### change logs
postgres 13
- now you can add partitioned tables to publication, then all partitions (include future partitions) are added
- supports `publish_via_partition_root` option, if enabled, changes on partitions will be published on partitioned
  table which means that the replication side can use a non-partitioned table to subscribe

### FAQ
What happens if I remove a subscribed table on replication side and add it again?

    The subscription won't continue. You need to use `alter subscription SUB_NAME refresh publication;`
    to refresh the subscription. I think the subscription binds with oid but not name internally.


### references
- [pg doc: create subscription](https://www.postgresql.org/docs/12/sql-createsubscription.html)
