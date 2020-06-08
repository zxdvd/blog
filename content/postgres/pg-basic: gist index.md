```metadata
tags: postgres, pg-basic, index, gist
```

## pg-basic: gist index

One advantage of postgres is that it supports many index methods, and you can write your
 own index method if you want. GiST is a built-in index that mainly used for multi-dimensional
 data. The underground data structure is r-tree.

Let's create a test table:

```sql
postgres@localhost:test_gist> create table t2  as select x as start, x + make_interval(hours => (random()*10)::int) as end
from generate_series('2018-01-01','2020-01-01', interval '1 hour') x;
```

A simple range overlap query needs to scan the whole table.

```sql
postgres@localhost:test_gist> explain analyze select * from t2 where tstzrange(start, "end") && tstzrange('2019-03-01', '2019-03-02')
+---------------------------------------------------------------------------------------------------------+
| QUERY PLAN                                                                                              |
|---------------------------------------------------------------------------------------------------------|
| Seq Scan on t2  (cost=0.00..358.62 rows=176 width=16) (actual time=4.105..7.350 rows=25 loops=1)        |
|   Filter: (tstzrange(start, "end") && '["2019-03-01 00:00:00+08","2019-03-02 00:00:00+08")'::tstzrange) |
|   Rows Removed by Filter: 17496                                                                         |
| Planning time: 0.157 ms                                                                                 |
| Execution time: 7.397 ms                                                                                |
+---------------------------------------------------------------------------------------------------------+
```

Add a gist index and do the same query again:

```sql
postgres@localhost:test_gist> create index t2_gist1 on t2 using gist(tstzrange(start, "end" ) )
CREATE INDEX
Time: 1.026s (a second), executed in: 1.026s (a second)
postgres@localhost:test_gist> explain analyze select * from t2 where tstzrange(start, "end") && tstzrange('2019-03-01', '2019-03-02')
+--------------------------------------------------------------------------------------------------------------------+
| QUERY PLAN                                                                                                         |
|--------------------------------------------------------------------------------------------------------------------|
| Bitmap Heap Scan on t2  (cost=9.63..108.65 rows=175 width=16) (actual time=0.093..0.140 rows=25 loops=1)           |
|   Recheck Cond: (tstzrange(start, "end") && '["2019-03-01 00:00:00+08","2019-03-02 00:00:00+08")'::tstzrange)      |
|   Heap Blocks: exact=2                                                                                             |
|   ->  Bitmap Index Scan on t2_gist1  (cost=0.00..9.59 rows=175 width=0) (actual time=0.081..0.082 rows=25 loops=1) |
|         Index Cond: (tstzrange(start, "end") && '["2019-03-01 00:00:00+08","2019-03-02 00:00:00+08")'::tstzrange)  |
| Planning time: 0.855 ms                                                                                            |
| Execution time: 0.285 ms                                                                                           |
+--------------------------------------------------------------------------------------------------------------------+
```

It uses the gist index with a bitmap scan. It's about 25 times faster.


### btree_gist

By default, it only supports few data types: range, box, circle, jsonb and others. It doesn't
 support int, text. We can get all supported data types of a index method using following
 sql:

```sql
postgres@localhost:test_gist> select a.oid, a.amname, a.amhandler, o.opfname from pg_am a, pg_opfamily o
    where a.oid = o.opfmethod and a.amname = 'gist';
+-------+----------+-------------+--------------+
| oid   | amname   | amhandler   | opfname      |
|-------+----------+-------------+--------------|
| 783   | gist     | gisthandler | network_ops  |
| 783   | gist     | gisthandler | box_ops      |
| 783   | gist     | gisthandler | poly_ops     |
| 783   | gist     | gisthandler | circle_ops   |
| 783   | gist     | gisthandler | point_ops    |
| 783   | gist     | gisthandler | tsvector_ops |
| 783   | gist     | gisthandler | tsquery_ops  |
| 783   | gist     | gisthandler | range_ops    |
| 783   | gist     | gisthandler | jsonb_ops    |
+-------+----------+-------------+--------------+
```

 But we can install the `btree_gist` extension then it will support these types.
`btree_gist` is an official extension. We can install it using sql `create extension btree_gist;`.
After installation, above query got 34 rows on my machine. It supports int, float, date related
types.

Attention, you need superuser permission to create extension and extension is only installed
 to the current database.

Now we can create gist index on int type.

```sql
postgres@localhost:test_gist> create table t3 as select x, x + (50*random())::int as y  from generate_series(1, 100000) x
SELECT 100000
Time: 0.176s
postgres@localhost:test_gist> create index t3_gist1 on t3 using gist(x,y)
CREATE INDEX
Time: 2.305s (2 seconds), executed in: 2.305s (2 seconds)
postgres@localhost:test_gist> explain analyze select * from t3 where y between 80000 and 80100
+---------------------------------------------------------------------------------------------------------------------+
| QUERY PLAN                                                                                                          |
|---------------------------------------------------------------------------------------------------------------------|
| Bitmap Heap Scan on t3  (cost=5.32..247.10 rows=101 width=8) (actual time=0.098..0.192 rows=106 loops=1)            |
|   Recheck Cond: ((y >= 80000) AND (y <= 80100))                                                                     |
|   Heap Blocks: exact=2                                                                                              |
|   ->  Bitmap Index Scan on t3_gist1  (cost=0.00..5.29 rows=101 width=0) (actual time=0.085..0.086 rows=106 loops=1) |
|         Index Cond: ((y >= 80000) AND (y <= 80100))                                                                 |
| Planning time: 0.229 ms                                                                                             |
| Execution time: 0.332 ms                                                                                            |
+---------------------------------------------------------------------------------------------------------------------+
```

### gist vs btree

`btree` is the default index type. And I think it is the most comman index type in most
 databases. It seems that both `b-tree` and `gist` support indexing like `(x,y,z)`. So
 what's the difference?

The main difference is that `b-tree` is single dimension while `gist` is for multi-dimension.

So for a b-tree index on `(x,y,z)`, it can only be used when the query condition **matched
 from head**, like `where x=1 and y in (1,2,3)`, `where x > 10 and x < 15`. It won't work
 for queries like `where y=1 and z=2` since it has no condition about `x`.

However, multi-dimension index supports query condition that mached with any dimension.
For a gist index on `(x,y,z)`, it supports query like `where y=1`, `where y in (1,2) and z<10`.

If your query condition is always matched from head of the composed columns, b-tree is better.
Since a condition may match many bounding boxes, you need to scan more nodes and bitmap scan
 is slower.

