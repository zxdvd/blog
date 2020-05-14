<!---
tags: postgres, internal, storage, mvcc
-->

## pg-internal: MVCC
MVCC is short for `Multi-version Concurrency Control`. It means multiple clients can
 access the database concurrently. Each will see a consistent snapshot of data and
 read won't block write while write won't block read. Other database like Oracle,
 mysql also support MVCC but the implementation may be different.

### MVCC implementation
For each row, postgres won't overwrite the row inplace when updating. It will mark the
 old row as deleted and insert new row in other place. By this way, old transaction can
 use old row and new transaction will use new row for same logical row at the same time.

Then a simple row (id=1, name=James) may have many physical rows in database files if it
 was updated multiple times. But at a specific point, only one physical row is visible
 for a transaction. So how to know whether a row is visible or not?

For each row (tuple), postgres added some metadata fields, such as t_ctid, t_xmin, t_xmax
 and other fields. `t_ctid` is physical index of each tuple. For example, (0, 1) means
 first tuple of first page. `t_ctid` may point to new tuple when deprecated.

The `t_xmin` is the ID of transaction that inserted itself. And the `t_xmax` is the ID of
 transaction that deprecated itself by deleting or updating. 0 means not deprecated.

Remember that the transaction may be ROLLBACKed. So you need to check clog to get status
 of the transaction. If the transaction of the `t_xmin` is rollbacked, then it means the
 inserting is rollbacked. And same to `t_xmax`.

So with `t_xmin` and `t_xmax` and other info like transaction status, we can decided that
 whether a tuple is visible to current transaction.

We can select `ctid`, `xmin` and `xmax` as normal fields but they are not contained in the
 `*`.

```sql
postgres@localhost:test1> select ctid,xmax,xmin, * from t1;
+--------+---------+---------+------+---------------+
| ctid   | xmax    | xmin    | id   | name          |
|--------+---------+---------+------+---------------|
| (0,1)  | 0       | 9219118 | 1    | abc           |
| (0,2)  | 9219122 | 9219121 | 2    | test rollback |
+--------+---------+---------+------+---------------+
```

Since it is a normal sql, it cannot see invalid tuples. We can use the extension `pageinspect`
 to get raw tuple info of page.

```sql
postgres@localhost:test1> create extension pageinspect;    -- install the extension

postgres@localhost:test1> SELECT lp,lp_off,lp_len,t_xmin,t_xmax,t_ctid,t_infomask2,t_infomask FROM heap_page_items(get_raw_page('t1', 0))
+------+----------+----------+----------+----------+----------+---------------+--------------+
| lp   | lp_off   | lp_len   | t_xmin   | t_xmax   | t_ctid   | t_infomask2   | t_infomask   |
|------+----------+----------+----------+----------+----------+---------------+--------------|
| 1    | 8160     | 32       | 9219118  | 0        | (0,1)    | 2             | 2306         |
| 2    | 8112     | 42       | 9219121  | 9219122  | (0,3)    | 16386         | 2306         |
| 3    | 8064     | 48       | 9219122  | 0        | (0,3)    | 32770         | 10754        |
+------+----------+----------+----------+----------+----------+---------------+--------------+
```

With this, we can see tuples that rollbacked or replaced.

### references
- [official doc: pageinspect](https://www.postgresql.org/docs/current/pageinspect.html)
