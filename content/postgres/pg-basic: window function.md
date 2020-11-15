<!---
tags: postgres, window function
-->

Window function looks similar to `group by` but varies greatly. The `group by` clause will 
aggregate each group (multiple rows) to a single row while the window function will compute 
the aggregation but it won't condense rows. You can use the aggregation result along with 
origin row, for example, comparing the result with some column. So it is often used in analysis 
situations.

Following is a simple example. It partitions the table via column product_id. Then computes 
average amount in each group and attach the result along with origin row.

Then you can compare the avg_amount with amount to analyze whether someone sales it too high 
or too low.

```
postgres@localhost:test> select id,product_id,amount, avg(amount) over w1 as avg_amount 
        from test."order" window w1 as (partition by product_id);
+------+--------------+----------+-----------------------+
| id   | product_id   | amount   | avg_amount            |
|------+--------------+----------+-----------------------|
| 5    | 1            | 9000     | 8333.3333333333333333 |               group 1
| 1    | 1            | 8000     | 8333.3333333333333333 |               group 1
| 4    | 1            | 8000     | 8333.3333333333333333 |               group 1
| 6    | 2            | 5000     | 6666.6666666666666667 |               group 2
| 2    | 2            | 8000     | 6666.6666666666666667 |               group 2
| 7    | 2            | 7000     | 6666.6666666666666667 |               group 2
| 8    | 3            | 7500     | 7666.6666666666666667 |               group 3
| 9    | 3            | 8000     | 7666.6666666666666667 |               group 3
| 3    | 3            | 7500     | 7666.6666666666666667 |               group 3
+------+--------------+----------+-----------------------+
```

### basic syntax
For above query, you can also write like following

    select id,product_id,amount, avg(amount) over (partition by product_id) as avg_amount from test."order";

You can write full window definition after `over` but I'd suggest to define window so that we 
can reuse it.

    select id,product_id,amount, avg(amount) over w1, count(*) over w1, sum(amount) over w1
    from test."order" window w1 as (partition by product_id);

You can take all rows as a single partition via `over()` or `w1 as ()`. Following get average 
amount across all rows.

    select *, avg(amount) over w1  from test."order" window w1 as ();

You can use multiple windows and one window can inherit another (you can only add `order by` 
clause if parent don't have).

``` sql
postgres@localhost:test> select id,product_id,sale_id,amount
           ,avg(amount) over w1 as product_avg_amount
           ,first_value(sale_id) over w2 as first_sale_id
           ,avg(amount) over w3 as sale_avg_amount
    from test."order"
    window w1 as (partition by product_id)
          ,w2 as (w1 order by id)      -- inherit w1, same as (partition by product_id order by id)
          ,w3 as (partition by sale_id);

+------+--------------+-----------+----------+-----------------------+-----------------+-----------------------+
| id   | product_id   | sale_id   | amount   | product_avg_amount    | first_sale_id   | sale_avg_amount       |
|------+--------------+-----------+----------+-----------------------+-----------------+-----------------------|
| 1    | 1            | 1         | 8000     | 8333.3333333333333333 | 1               | 8333.3333333333333333 |
| 4    | 1            | 1         | 8000     | 8333.3333333333333333 | 1               | 8333.3333333333333333 |
| 5    | 1            | 1         | 9000     | 8333.3333333333333333 | 1               | 8333.3333333333333333 |
| 9    | 3            | 2         | 8000     | 7666.6666666666666667 | 3               | 6833.3333333333333333 |
| 6    | 2            | 2         | 5000     | 6666.6666666666666667 | 3               | 6833.3333333333333333 |
| 8    | 3            | 2         | 7500     | 7666.6666666666666667 | 3               | 6833.3333333333333333 |
| 3    | 3            | 3         | 7500     | 7666.6666666666666667 | 3               | 7500.0000000000000000 |
| 2    | 2            | 3         | 8000     | 6666.6666666666666667 | 3               | 7500.0000000000000000 |
| 7    | 2            | 3         | 7000     | 6666.6666666666666667 | 3               | 7500.0000000000000000 |
+------+--------------+-----------+----------+-----------------------+-----------------+-----------------------+
```

### order by and rank
Sometimes we need to rank in each partition. And for rank, `order by` is a must-have. Postgres provides
 several functions for rank. See following example:

```sql
postgres@localhost:test> select id,amount
                ,row_number() over w1
                ,rank() over w1
                , dense_rank() over w1
                from test."order" win dow w1 as (order by amount desc);
+------+----------+--------------+--------+--------------+
| id   | amount   | row_number   | rank   | dense_rank   |
|------+----------+--------------+--------+--------------|
| 5    | 9000     | 1            | 1      | 1            |
| 4    | 8000     | 2            | 2      | 2            |
| 2    | 8000     | 3            | 2      | 2            |
| 9    | 8000     | 4            | 2      | 2            |
| 1    | 8000     | 5            | 2      | 2            |
| 8    | 7500     | 6            | 6      | 3            |
| 3    | 7500     | 7            | 6      | 3            |
| 7    | 7000     | 8            | 8      | 4            |
| 6    | 5000     | 9            | 9      | 5            |
+------+----------+--------------+--------+--------------+
```

We can find that `row_number()` is ranked increased by one that started from 1. The `rank()` changes 
a little. It gives same rank result for those have same values (all of id 4,2,9,1 are rank 2 since 
amount is 8000). Hence, there may have holes in the rank sequence. Then came the `dense_rank()`. It 
also gives same rank to those have same result but it won't have holes. At above example, id 8 is 
ranked 6 by `rank()` while ranked 3 by `dense_rank()`.

### window and frame
Window function aggregated on full partition by default. But you may want to aggregate partially in 
the partition. Let's see following example:

```sql
postgres@localhost:test> select id,amount
            ,count(*) over w1 as count_w1, sum(amount) over w1 as sum_w1
            ,count(*) over w2 as count_w2, sum(amount) over w2 as sum_w2
            ,count(*) over w3 as count_w3, sum(amount) over w3 as sum_w3
        from test."order"
        window w1 as ()
              ,w2 as (order by id rows unbounded preceding)
              ,w3 as (order by id rows between 1 preceding and 1 following);
+------+----------+------------+----------+------------+----------+------------+----------+
| id   | amount   | count_w1   | sum_w1   | count_w2   | sum_w2   | count_w3   | sum_w3   |
|------+----------+------------+----------+------------+----------+------------+----------|
| 1    | 8000     | 9          | 68000    | 1          | 8000     | 2          | 16000    |
| 2    | 8000     | 9          | 68000    | 2          | 16000    | 3          | 23500    |
| 3    | 7500     | 9          | 68000    | 3          | 23500    | 3          | 23500    |
| 4    | 8000     | 9          | 68000    | 4          | 31500    | 3          | 24500    |
| 5    | 9000     | 9          | 68000    | 5          | 40500    | 3          | 22000    |
| 6    | 5000     | 9          | 68000    | 6          | 45500    | 3          | 21000    |
| 7    | 7000     | 9          | 68000    | 7          | 52500    | 3          | 19500    |
| 8    | 7500     | 9          | 68000    | 8          | 60000    | 3          | 22500    |
| 9    | 8000     | 9          | 68000    | 9          | 68000    | 2          | 15500    |
+------+----------+------------+----------+------------+----------+------------+----------+
```

Now I'll explain the result. The `count_w1` and `sum_w1` columns are normal window aggregation
 like those in above section.

The next 4 columns are aggregated on different frames. A frame is a consecutive subset within a
 partition. A frame has start and end. You can define the start or end point using following
methods:

    unbounded preceding              means START of the partition inclusive
    unbounded following              means end of the partition inclusive
    current row                      means the current row computed inclusive
    N preceding                      means N rows/range/groups before the current row inclusive
    N following                      means N rows/range/groups after the current row inclusive

So the window `w2 as (order by id rows unbounded preceding)` defines a frame that from the start 
of the partition to current row (end will be current row if omitted).

So when current row is first row (id=1), the frame only has one row thus first row, then count_w2 
is 1 and sum_w2 is 8000. When current row is second row (id=2), the frame contains the first two 
rows. So count_w2 is 2 and sum_w2 is 8000+8000=16000. And so on, when current row is the last row 
(id=9), the frame contains all rows, then count_w2 and sum_w2 equal count_w1 and sum_w1.

The window `w3 as (order by id rows between 1 preceding and 1 following)` defines a frame that 
contains 3 rows: the current row, the one before it and the one after it.

So it's easy to understand the count_w3. For the first row count_w3 is 2 since this frame has no 
`the one before it`. And same for the last row since there is no row after it. For all in the middle,
the count_w3 are 3. The sum_w3 is sum of the frame.

Window frame `rows unbounded preceding` is often used when you want to calculate accumulated result 
from start of year or start of month. And you can use window frame `rows N preceding` to calculate 
daily, weekly, monthly average of stock.

Attention, NOT ALL window functions are calculated within the frame. It depends on the implementation 
of the window function. Some will just ignore it and keep calculating on the whole partition even 
though you've defined a frame. See following example:

```sql
postgres@localhost:test> select id
            ,first_value(id) over w1 as first_w1, row_number() over w1
            ,first_value(id) over w2 as first_w2, row_number() over w2
            ,first_value(id) over w3 as first_w3, row_number() over w3
        from test."order"
        window w1 as ()
              ,w2 as (order by id rows unbounded preceding)
              ,w3 as (order by id rows between 1 preceding and 1 following);
+------+------------+--------------+------------+--------------+------------+--------------+
| id   | first_w1   | row_number   | first_w2   | row_number   | first_w3   | row_number   |
|------+------------+--------------+------------+--------------+------------+--------------|
| 1    | 2          | 9            | 1          | 1            | 1          | 1            |
| 2    | 2          | 1            | 1          | 2            | 1          | 2            |
| 3    | 2          | 2            | 1          | 3            | 2          | 3            |
| 4    | 2          | 3            | 1          | 4            | 3          | 4            |
| 5    | 2          | 4            | 1          | 5            | 4          | 5            |
| 6    | 2          | 5            | 1          | 6            | 5          | 6            |
| 7    | 2          | 6            | 1          | 7            | 6          | 7            |
| 8    | 2          | 7            | 1          | 8            | 7          | 8            |
| 9    | 2          | 8            | 1          | 9            | 8          | 9            |
+------+------------+--------------+------------+--------------+------------+--------------+
```

From the above result we can find that `row_number()` ignores the frame. It gives result base 
on the whole partition. You may find that the `row_number()` on w1 is unordered, that's because 
we didn't define `order by` in window w1 then it can be any order.

The `first_value()` is frame aware. Otherwise, it should always give 1 on w2 and w3.

Then a simple summary:

    frame aware window functions:
        count, sum, avg, first_value, last_value
    frame unaware window functions:
        row_number, rank, dense_rank

### frame mode: rows, range, groups
You can define window frame like `rows between XXX` or `range between XXX`. So what's 
the difference of rows mode and range mode?

Before answer this, I need to explain a word `peer`. Suppose a window define as 
`order by user_id`. Then the user_ids in a partition may be `1,2,3,3,6,7,7,7,10,11,11`.
The consecutive rows in the partition that have same user_id are peers. Thus `3,3`, 
`7,7,7` and `11,11` are peers.

In rows mode, the start and end of frame are defined at rows precision. They are either
 start of partition or end of partition, or +/- N rows of current row.

For range mode, it has following differences:

- the start and end are **expanded** to include **peers**
- the N of `N preceding` and `N following` means value that is same type as the `order by`

See following example:

```sql
postgres@localhost:test> select id,amount
                  ,count(*) over w1 as count_w1
                  ,count(*) over w2 as count_w2
                  ,count(*) over w3 as count_w3
              from test."order"
              window
                   w1 as (order by amount range 10 preceding)
                  ,w2 as (order by amount range between 1000 preceding and current row )
                  ,w3 as (order by amount rows between current row and 2 following);
+------+----------+------------+------------+------------+
| id   | amount   | count_w1   | count_w2   | count_w3   |
|------+----------+------------+------------+------------|
| 6    | 5000     | 1          | 1          | 3          |
| 7    | 7000     | 1          | 1          | 3          |
| 3    | 7500     | 2          | 3          | 3          |
| 8    | 7500     | 2          | 3          | 3          |
| 1    | 8000     | 4          | 7          | 3          |
| 4    | 8000     | 4          | 7          | 3          |
| 2    | 8000     | 4          | 7          | 3          |
| 9    | 8000     | 4          | 7          | 2          |
| 5    | 9000     | 1          | 5          | 1          |
+------+----------+------------+------------+------------+
```

The frame of `count_w3` is current row and following two rows. So for the last row, the count
 is 1 since it has no following rows. And for second last, the result is 2. For all others,
 the result is 3 since they all have at least two following rows.

The `count_w1` counts rows that `amount` is between `[current_amount-10, current_amount]` inclusive.
So for `amount` 5000, 7000, 9000 the result is 1 since they only have themself in this range. For
 `amount` 7200, since range mode will expand `peers`, then for id 3 and 8, the result is 2.

Same as `count_w1`, the `count_w1` counts rows that `amount` is between
 `[current_amount-1000, current_amount]` inclusive. Then for id 3 and 8, the result is 3 since
 amount 7000 is included. And for amount 8000, the result is 7 since 7500 in included.

So why range mode? Since rows in peer are same. Then their order are not determined. For above example,
it means that id=3 may be the third row but id=8 can also be third row. Then for id=3 and id=8, the 
count may be 3 or 4. The result is not determined. This is same to other window function. You may not 
want this. We hate uncertainness.

To avoid this uncertainness, we can use range mode. Or we can eliminate the peer via using unique columns 
in the `order by clause`, e.g., `order by amount, id`, since id is unique.

#### groups
From postgres 11, it supports a new frame mode: `groups`. It likes a combination of `rows` and `range`.
It will expand `peers` just like `range`. And the N in `N preceding/following` means number of `peers`
 before and after current row.

I exported the small table used above to a csv. You can download it from [here](./misc/order-small.csv).

### references
- [official doc: sql expression - window function calls](https://www.postgresql.org/docs/10/sql-expressions.html#SYNTAX-WINDOW-FUNCTIONS)
- [official doc: list of window functions](https://www.postgresql.org/docs/10/functions-window.html)
