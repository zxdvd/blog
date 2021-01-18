```metadata
tags: postgres, pg stat
```

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
select x, x as y into test_stat from generate_series(1, 10000) x;
alter table test_stat set (autovacuum_enabled=off);
-- do a update to generate some dead tuples
update  test_stat set y = x%10 where x%10 <= 3;

-- now have a look of the table stat
postgres@localhost:test1> select seq_scan, seq_tup_read, idx_scan, idx_tup_fetch, n_live_tup, n_dead_tup
            from pg_stat_user_tables where relname = 'test_stat';
+------------+----------------+------------+-----------------+--------------+--------------+
| seq_scan   | seq_tup_read   | idx_scan   | idx_tup_fetch   | n_live_tup   | n_dead_tup   |
|------------+----------------+------------+-----------------+--------------+--------------|
| 1          | 10000          | <null>     | <null>          | 10000        | 4000         |
+------------+----------------+------------+-----------------+--------------+--------------+
```

In above result, the n_live_tup and n_dead_tup are expected since I've updated 10000 * 4/10 =2500 rows.
 The seq_scan and seq_tup_read are also easy to understand.

    I've done a full table `UPDATE`. It did a sequence scan and read all tuples.
    So seq_scan+=1, seq_tup_read+=10000

Let's verify it by executing a `select * from test_stat limit 123;`. Then expected stat should be 
seq_scan=2 and seq_tup_read=10000+123=10123. And I got same result.

Does seq_tup_read count dead tuples?

I did a simple `select count(*) from test_stat;`. And seq_scan became 3 while seq_tup_read became 20123.
 Therefore, I'm sure that dead tuples were not counted into seq_tup_read and that's why I did a update at
 first to generate dead tuples.

### index stat
I'll reset the stat and add index for above table.

``` sql
-- this function can also be used to reset stat of a index
select pg_stat_reset_single_table_counters('test_stat'::regclass);
-- add a primary key, this will create a btree index automatically

alter table test_stat add primary key (x);
-- it seems the `add primary key` did two full table scan. Now seq_scan=2, seq_tup_read=20000

-- disable seqscan to force use index scan, you can use explain to check the plan of the query at first
set enable_seqscan=false;

-- test a simple query and then check the stat
select * from test_stat where x >9000 limit 20;

postgres@localhost:test1> select t1.seq_scan,t1.seq_tup_read,t1.idx_scan as t_idx_scan
            ,t1.idx_tup_fetch as t_idx_tup_fetch,i1.idx_scan,i1.idx_tup_read,i1.idx_tup_fetch
        from pg_stat_user_tables t1, pg_stat_all_indexes i1
        where t1.relname = i1.relname and t1.relname = 'test_stat'
+------------+----------------+--------------+-------------------+------------+----------------+-----------------+
| seq_scan   | seq_tup_read   | t_idx_scan   | t_idx_tup_fetch   | idx_scan   | idx_tup_read   | idx_tup_fetch   |
|------------+----------------+--------------+-------------------+------------+----------------+-----------------|
| 2          | 10000          | 1            | 20                | 1          | 20             | 20              |
+------------+----------------+--------------+-------------------+------------+----------------+-----------------+
```

It seems that idx_scan and idx_tup_read of index stat are same like seq_scan and seq_tup_read of  table stat. 
And it's easy to understand that idx_scan and idx_tup_fetch in table stat are sums of those in all indexes of 
this table.

However, what's the difference between idx_tup_read and idx_tup_fetch?

They are same in above result but we can find that in other real word tables, they are always different. Let's
 see bitmap scan.

### a bitmap scan case
I'll reset stat of table and index and test a simple bitmap scan.

```sql
explain (analyze, costs false) select count(*) from test_stat where x > 8000 or x < 500
+----------------------------------------------------------------------------------------------------+
| QUERY PLAN                                                                                         |
|----------------------------------------------------------------------------------------------------|
| Aggregate (actual time=3.841..3.841 rows=1 loops=1)                                                |
|   ->  Bitmap Heap Scan on test_stat (actual time=0.195..2.104 rows=2499 loops=1)                   |
|         Recheck Cond: ((x > 8000) OR (x < 500))                                                    |
|         Heap Blocks: exact=18                                                                      |
|         ->  BitmapOr (actual time=0.169..0.169 rows=0 loops=1)                                     |
|               ->  Bitmap Index Scan on test_stat_pkey (actual time=0.138..0.138 rows=2000 loops=1) |
|                     Index Cond: (x > 8000)                                                         |
|               ->  Bitmap Index Scan on test_stat_pkey (actual time=0.027..0.027 rows=499 loops=1)  |
|                     Index Cond: (x < 500)                                                          |
| Planning time: 0.164 ms                                                                            |
| Execution time: 3.911 ms                                                                           |
+----------------------------------------------------------------------------------------------------+

select t1.seq_scan,t1.seq_tup_read,t1.idx_scan as t_idx_scan
             ,t1.idx_tup_fetch as t_idx_tup_fetch,i1.idx_scan,i1.idx_tup_read,i1.idx_tup_fetch
         from pg_stat_user_tables t1, pg_stat_all_indexes i1
         where t1.relname = i1.relname and t1.relname = 'test_stat'

+------------+----------------+--------------+-------------------+------------+----------------+-----------------+
| seq_scan   | seq_tup_read   | t_idx_scan   | t_idx_tup_fetch   | idx_scan   | idx_tup_read   | idx_tup_fetch   |
|------------+----------------+--------------+-------------------+------------+----------------+-----------------|
| 0          | 0              | 2            | 2499              | 2          | 2499           | 0               |
+------------+----------------+--------------+-------------------+------------+----------------+-----------------+
```

This query did a BitmapOr on result of two index scan. So the idx_scan is 2. We can find that idx_tup_fetch
 is **0** and idx_tup_fetch of TABLE is 2499. That's because the Bitmap scan is something like the sequence scan.
 Each page of the table is a bit in the bitmap. It reads the index tuples and set related bit of the bitmap if
 a tuple matches the condition. It doesn't return CTIDs of any table tuples so that the `idx_tup_fetch` of index
 is 0. And postgres counts bitmap as scan fetch to stat of table.

Summary:
-. `idx_scan` and `seq_scan` count how many times the relation is scanned
-. `idx_tup_read` counts tuples of an index read
-. `idx_tup_fetch` of index counts tuples of table found by the reading of the index
-. `idx_tup_fetch` of table counts tuples of table fetched by index scan (including bitmap scan)

### references
- [official doc: monitoring-stats](https://www.postgresql.org/docs/10/monitoring-stats.html)
