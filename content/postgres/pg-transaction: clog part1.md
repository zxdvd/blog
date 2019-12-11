<!---
tags: postgres, transaction, clog
date: 2019-12-11
-->

There is a lot of articles about MVCC and wallog but not so much about clog.
So I'd like to write something about clog.

I won't explain it too much about MVCC since I will write another blog about this.
We knew that postgres implementing update as `delete old and insert a new` while
 delete as `mark tuple as delete`. Examples

```
# suppose transaction 100 insert a row, then we have
    xmin    xmax    ctid    id      name
    100     0       (0,1)    1      Tom

# xmin is the transactionId that insert this tuple, when we update it we got following
    xmin    xmax    ctid    id      name
    100     101     (0,1)    1      Tom
    101     0       (0,2)    1      David

# xmax is the transactionId that mark it as invalid

# then if we delete it in a new transction, we got following
    xmin    xmax    ctid    id      name
    100     101     (0,1)    1      Tom
    101     102     (0,2)    1      David
```

Actually, there is more details behind it. From above example, can we simply concluded 
that **a tuple has xmax==0 is visible and xmax!=0 is invisible**?

No. We don't know whether the transaction is rollbacked or not. Tuple that was inserted 
by rollbacked transction is invalid and the `marking delete` of rollbacked transction 
is also invalid.

Then we know that

    to check tuple visibility we may need to know whether a transction is committed or not

So how to check commit or rollback info about a transction? The answer is **clog**.

Postgres defines 4 phases of transction using 2 bits. They are 
`IN_PROGRESS, COMMITTED, ABORTED, SUB_COMMITTED`. It uses a large bitmap to hold status 
of each transction. So bit 0-1 is for transaction 0, 2-3 for transction 1, 100-101 for 
transction 50, and so on (let's ignore the special transactionIds).

The transctionId is 32 bit integer then we may need 2*2^32 bits(1GiB) to store this bitmap. 
Postgres stores it as multiple files since it's too large to be stored in memory and in most 
time we only need to get statuses of recent transactions.

Each file is at most 256kb, thus 32 blocks. They are stored at `DATA_DIR/pg_xact` 
(before postgres 10, it's `pg_clog`). Filenames are 0000,0001,...,000E,000F,0010,...,0FFF.

Checking transction status is such called so frequently so that it must be very fast. Postgres 
has a in memory LRU cache to store hot blocks from these files.

The checking is fast if it hit the cache. Otherwise, it will be very slow since it needs to 
do file IO. So postgres saves the checking result in infomask field of each tuple as hint bit. 
Then there is no need to check from clog. (In version before 9.4, it set xmin to a special 
transactionId as a hint).

During vacuum, it will also freeze some tuples accordingly. The freezing is setting hint bit 
of tuple like above to avoid checking clog.


### references
[www.interdb.jp/pg/pgsql05.html](http://www.interdb.jp/pg/pgsql05.html#_5.4.1)
[official doc: vaccum](https://www.postgresql.org/docs/10/static/routine-vacuuming.html)
