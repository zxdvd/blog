<!---
tags: database, postgres, cte, materialize
date: 2020-01-05
-->

Recently, I planed to upgrade our warehouse from postgres 10 to postgres 12. However, after setup 
and migrate data, I found that our ETL tasks became slower. It used to be done within half hour.
But at postgres 12, it used more then 2 hours.

Then I use pgbench to run some benchmarks on the two server. The result shows that postgres 12 is 
a little faster than postgres 10. I think maybe something wrong with my ETL scripts. After analyzing 
a single script for multiple times, I identified that problem is the WITH CTE statements.

Yes, postgres 12 did have a large change to the WITH CTE statement.

Before postgres 12, the WITH CTE statements are materialized (just like temporary table). There are 
some disadvantages:

    * materialize is very slow
    * there may be better plan if take the CTE as subquery
    * after materialized, it cannot use any index of the origin tables

A simple example

```sql
explain with t1 as (select * from student)
select * from t1 order by id limit 10;
+----------------------------------------------------------------------------------+
| QUERY PLAN                                                                       |
|----------------------------------------------------------------------------------|
| Limit  (cost=311862.50..311862.52 rows=10 width=1362)                            |
|   CTE t1                                                                         |
|     ->  Seq Scan on student (cost=0.00..164266.57 rows=3547157 width=1107) |
|   ->  Sort  (cost=147595.93..156463.82 rows=3547157 width=1362)                  |
|         Sort Key: t1.id                                                          |
|         ->  CTE Scan on t1  (cost=0.00..70943.14 rows=3547157 width=1362)        |
+----------------------------------------------------------------------------------+
```

Due to the `materialize`, such a simple query cannot use the primary key index. It will materialize 
the whole table and then sort it and output 10 rows. It will be very very slow.

It also has some advantanges on the other hand. It provides a way to change the plan. There care cases 
that planner didn't find the best plan, especially for large tables the statistics may not be sufficient.

From postgres 12, it will try to fit the CTE into the whole query to get a plan. Let's see the plan of 
above query:

```sql
explain with t1 as (select * from student)
select * from t1 order by id limit 10;
+-------------------------------------------------------------------------------------------------+
| QUERY PLAN                                                                                      |
|-------------------------------------------------------------------------------------------------|
| Limit  (cost=0.43..1.70 rows=10 width=704)                                                      |
|   ->  Index Scan using student_id on student (cost=0.43..360024.08 rows=2832398 width=704) |
+-------------------------------------------------------------------------------------------------+
```
Now the CTE can use indexes and this will be hundred times faster.

However, you'll get trouble if you've used `WITH CTE` as a way to change the plan. Postgres 12 also 
provides a new syntax to help to control the `materialize` behavior.

You can use `with t1 as MATERIALIZED (...)` to tell the planner that you want to materialize it. 
You can also use `with t1 as NOT MATERIALIZED (...)` to hint that you want it to be inlined.

Following is the source code to control this behavior

```C
// src/backend/optimizer/plan/subselect.c

             if ((cte->ctematerialized == CTEMaterializeNever ||
                      (cte->ctematerialized == CTEMaterializeDefault &&
                       cte->cterefcount == 1)) &&
                     !cte->cterecursive &&
                     cmdType == CMD_SELECT &&
                     !contain_dml(cte->ctequery) &&
                     (cte->cterefcount <= 1 ||
                      !contain_outer_selfref(cte->ctequery)) &&
                     !contain_volatile_functions(cte->ctequery))
             {
                     inline_cte(root, cte);
                     /* Make a dummy entry in cte_plan_ids */
                     root->cte_plan_ids = lappend_int(root->cte_plan_ids, -1);
                     continue;
             }
```

Back to my ETL cases, I don't want to change all the scripts since I need to do other tests on base 
postgres 10 and postgres 12 (new syntax will get error on postgres 10). I modified the source code 
to make the default behavior as `materialize`. I think maybe there should have a setting like 
`default_cte_materialized`.

### references
[official doc: with query](https://www.postgresql.org/docs/current/queries-with.html)
[mailist discussion: inline CTEs](https://www.postgresql.org/message-id/87sh48ffhb.fsf%40news-spur.riddles.org.uk)

