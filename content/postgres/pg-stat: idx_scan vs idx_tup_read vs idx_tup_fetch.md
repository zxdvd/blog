<!---
tags: postgres, pg stat
-->

We often need to know which index is frequently used and which is rarely used then we 
can change indexes of table.

We can get statistics about accessing to table or index from `pg_stat_all_tables` and 
`pg_stat_all_indexes`.

For the view `pg_stat_all_tables`, it has information about accessing to the table and 
indexes of this table, inserted rows, deleted rows, updated rows, live rows and dead 
rows, time and count of vacuum and analyze. However, we'll focus on following fields:

    seq_scan, seq_tup_read, idx_scan, idx_tup_fetch

For view `pg_stat_all_indexes`, it has fewer fields: idx_scan, idx_tup_read, idx_tup_fetch.


### table stat
I'll prepare a simple table and disable autovacuum.

``` sql
postgres@localhost:test1> select x into test_stat from generate_series(1, 5000) x;
postgres@localhost:test1> alter table test_stat set (autovacuum_enabled=off);
-- do a update to generate some dead tuples
postgres@localhost:test1> update  test_stat set x = x where x%4 = 1;

-- now have a look of the table stat
postgres@localhost:test1> select seq_scan, seq_tup_read, idx_scan, idx_tup_fetch, n_live_tup, n_dead_tup
            from pg_stat_user_tables where relname = 'test_stat';
+------------+----------------+------------+-----------------+--------------+--------------+
| seq_scan   | seq_tup_read   | idx_scan   | idx_tup_fetch   | n_live_tup   | n_dead_tup   |
|------------+----------------+------------+-----------------+--------------+--------------|
| 1          |  5000          | <null>     | <null>          | 5000         | 1250         |
+------------+----------------+------------+-----------------+--------------+--------------+
```

In above result, the n_live_tup and n_dead_tup are expected since I've updated 5000/4=1250 rows. The 
seq_scan and seq_tup_read are also easy to understand.

    I've done a `UPDATE`. It did a sequence scan and read all tuples. So seq_scan+=1, seq_tup_read+=5000

Let's verify it by executing a `select * from test_stat limit 123;`. Then expected stat should be 
seq_scan=2 and seq_tup_read=5000+123=5123. And I got same result.

Does seq_tup_read count dead tuples?

I did a simple `select count(*) from test_stat;`. And seq_scan became 3 while seq_tup_read became 10123. 
Therefore, I'm sure that dead tuples were not counted into seq_tup_read and that's why I did a update at 
first to generate dead tuples.

### index stat
I'll reset the stat and add index for above table.

``` sql
postgres@localhost:test1> select pg_stat_reset_single_table_counters('test_stat'::regclass);
-- add a primary key, this will create a btree index automatically
postgres@localhost:test1> alter table test_stat add primary key (x);
-- it seems the `add primary key` did two full table scan. Now seq_scan=2, seq_tup_read=10000


-- test a simple query and then check the stat
postgres@localhost:test1> select * from test_stat where x >4000 limit 11;

postgres@localhost:test1> select t1.seq_scan,t1.seq_tup_read,t1.idx_scan as t_idx_scan
            ,t1.idx_tup_fetch as t_idx_tup_fetch,i1.idx_scan,i1.idx_tup_read,i1.idx_tup_fetch
        from pg_stat_user_tables t1, pg_stat_all_indexes i1
        where t1.relname = i1.relname and t1.relname = 'test_stat'
+------------+----------------+--------------+-------------------+------------+----------------+-----------------+
| seq_scan   | seq_tup_read   | t_idx_scan   | t_idx_tup_fetch   | idx_scan   | idx_tup_read   | idx_tup_fetch   |
|------------+----------------+--------------+-------------------+------------+----------------+-----------------|
| 2          | 10000          | 1            | 11                | 1          | 11             | 11              |
+------------+----------------+--------------+-------------------+------------+----------------+-----------------+
```

It seems that idx_scan and idx_tup_read of index stat are same like seq_scan and seq_tup_read of  table stat. 
And it's easy to understand that idx_scan and idx_tup_fetch in table stat are sums of those in all indexes of 
this table.

However, what's the difference between idx_tup_read and idx_tup_fetch?

They are same in above result but we can find that in other real word tables, they are always different. Why?

``` sql
postgres@localhost:test1> select * from test_stat where x >4000 and y = 1 limit 11;

postgres@localhost:test1> select t1.seq_scan,t1.seq_tup_read,t1.idx_scan as t_idx_scan
            ,t1.idx_tup_fetch as t_idx_tup_fetch,i1.idx_scan,i1.idx_tup_read,i1.idx_tup_fetch
        from pg_stat_user_tables t1, pg_stat_all_indexes i1
        where t1.relname = i1.relname and t1.relname = 'test_stat'
+------------+----------------+--------------+-------------------+------------+----------------+-----------------+
| seq_scan   | seq_tup_read   | t_idx_scan   | t_idx_tup_fetch   | idx_scan   | idx_tup_read   | idx_tup_fetch   |
|------------+----------------+--------------+-------------------+------------+----------------+-----------------|
| 0          | 0              | 1            | 1000              | 1          | 1250           | 1000            |
+------------+----------------+--------------+-------------------+------------+----------------+-----------------+
```

This time the idx_tup_read is different from idx_tup_fetch. It seems that the dead tuples matter here.
I added the `y=1` in the query so that id cannot do index only scan. I have to check all the 1000 live 
tuples (x>4000). And it read more since there is about 1/4 dead tuples, 1000 * (1+1/4) = 1250.

### a bitmap scan case
I'll reset stat and test a simple bitmap scan.

```sql
postgres@localhost:test1> explain (analyze, costs false) select count(*) from test_stat where x > 4500 or x < 500
+---------------------------------------------------------------------------------------------------+
| QUERY PLAN                                                                                        |
|---------------------------------------------------------------------------------------------------|
| Aggregate (actual time=2.103..2.104 rows=1 loops=1)                                               |
|   ->  Bitmap Heap Scan on test_stat (actual time=0.164..1.174 rows=999 loops=1)                   |
|         Recheck Cond: ((x > 4500) OR (x < 500))                                                   |
|         Heap Blocks: exact=8                                                                      |
|         ->  BitmapOr (actual time=0.136..0.136 rows=0 loops=1)                                    |
|               ->  Bitmap Index Scan on test_stat_pkey (actual time=0.076..0.076 rows=500 loops=1) |
|                     Index Cond: (x > 4500)                                                        |
|               ->  Bitmap Index Scan on test_stat_pkey (actual time=0.055..0.055 rows=624 loops=1) |
|                     Index Cond: (x < 500)                                                         |
| Planning time: 0.229 ms                                                                           |
| Execution time: 2.195 ms                                                                          |
+---------------------------------------------------------------------------------------------------+

postgres@localhost:test1> select t1.seq_scan,t1.seq_tup_read,t1.idx_scan as t_idx_scan
            ,t1.idx_tup_fetch as t_idx_tup_fetch,i1.idx_scan,i1.idx_tup_read,i1.idx_tup_fetch
        from pg_stat_user_tables t1, pg_stat_all_indexes i1
        where t1.relname = i1.relname and t1.relname = 'test_stat'
+------------+----------------+--------------+-------------------+------------+----------------+-----------------+
| seq_scan   | seq_tup_read   | t_idx_scan   | t_idx_tup_fetch   | idx_scan   | idx_tup_read   | idx_tup_fetch   |
|------------+----------------+--------------+-------------------+------------+----------------+-----------------|
| 0          | 0              | 2            | 999               | 2          | 1124           | 0               |
+------------+----------------+--------------+-------------------+------------+----------------+-----------------+
```

This query did a BitmapOr on result of two index scan. So the idx_scan is 2. The result is 999 then 
idx_tup_read=1124 is reasonable considering about dead tuples.

However, we can find that idx_tup_fetch is **0** and idx_tup_fetch of TABLE is 999. That's because the 
Bitmap scan is something like the sequence scan. It did heap scan according the bitmap result. So no 
index tuple fetch. And postgres counts bitmap scan fetch to stat of table.


### references
- [official doc: monitoring-stats](https://www.postgresql.org/docs/10/monitoring-stats.html)
