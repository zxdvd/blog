```metadata
tags: postgres, pg-basic, datetime
```

## pg-basic: datetime tips

### generate datetime series
The `generate_series` can also be used to generate series of datetimes, like following examples:

    select generate_series(now(),now() + interval '2 day', interval '3 hour') date;
    select generate_series('2020-02-01','2020-02-02', interval '4 hour') date;

Both start and end are inclusive.


### begin and end of date
You can use `date_trunc(text, timestamp)` to get begin of date. Examples:

    select date_trunc('hour', now()), date_trunc('day', now()), date_trunc('week', now());
    select date_trunc('month', now()), date_trunc('year', now());

If you want to get end of day, you may try to convert it to `begin of next day minus 1 second`.

    select date_trunc('day', now()) + interval '7 day' - interval '1 sec';

This may not be accurate considering about time like `23:59:59.999`. Then you can
 change your query a little to avoid the `- interval '1 sec'`. If your query is
 `select * from t where created_at <= END_OF_DAY`, you may convert it to
 `select * from t where created_at < BEGIN_OF_DAY` so that no need to get end of day.

### now(), statement_timestamp(), clock_timestamp()
Let's see a example first. It will print current time, sleep few seconds and print it again.

```sql
DO $$
begin
    RAISE NOTICE 'now is %', now();
    perform pg_sleep(3);
    RAISE NOTICE 'now is %', now();
end $$ language plpgsql;
```

Then you can find that the second now is same as the first one. But if we simply executed
 two `select now();` in `psql`, the result is different. Why?

This is because `now()` == `transaction_timestamp()` == `CURRENT_TIMESTAMP`, they all mean
 start time of a transaction. So that when you `select now()` in a transaction, you always
 got same result.

If if really want the current time, you could try with `clock_timestamp()`, it will return
 the real current time from the system but it's slower than `now()` since `now()` is cached
 in transaction.

There is also a `statement_timestamp()`, it gives you start time of the statement. So it
 will keep same in the same statement.

### conversion
#### string to datetime
It's very easy to convert ISO8601 date time string.

```sql
    select '2000-01-01'::date;
```

Otherwise, the result may vary and depend on the `DateStyle` setting. The `select '01/02/03'::date`
 will be '2001-02-03' if DateStyle is `YMD`, '2003-01-02' if DateStyle is 'MDY'.

If you're not sure about the DateStyle setting or you want to write query that does not
 depend on this settings, you can also try with `to_date(text, text)` function.

```sql
    select to_date('1987300', 'YYYYDDD');          -- DDD is the day in the year, you'll get 1987-10-27
```

#### datetime to string
Just using `select now()::text;` to get ISO 8601 string of date time. You can also use
 the `to_char(timestamp, text)` function to format the result string.

