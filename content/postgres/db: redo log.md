<!---
tags: database, postgres, mysql, redo-log, wallog
-->

## database: redo log

Redo log, write ahead log (wal log) and similar mechanics are used in almost all kind 
of databases for following reasons:

  * data integrity, crash recovery
  * performance: make random writes to sequence writes
  * Incremental backup and PITR (pint in time recovery)
  * replication

#### data integrity, crash recovery
The server may work stably for most time but there is still chance that it crashed and 
we need to face it. Suppose that you are updating 10 rows, or 10 columns of a row? How 
do you know whether it succeeds of not after server crashes. It may be partially updated 
and data integrity is broked. And you have no way to recovery.

Then think about transaction, this problem is even more serious. It may lead to a transaction 
partially committed and you never know which committed and which not.

Redo log or write ahead log is a way to deal with this. First write all update/insert/delete 
actions into the redo log and assure the log was fsynced to disk. Then apply the actions one 
by one to the data file.

If server crashed in the first stage, then the transaction failed and nothing needed to do. 
If server crashed in the second stage, then you can recovery from the log since log has all 
the detailed actions.

#### performance
You may think that redo log will decrease server performance but actually not. Even with a 
system that can accept loss of very few data (suppose 1 second), then should we remove the 
redo log component to increase performance?

Of course you can, but it may not increase performance and even it may decrease it. If the 
system can tolerate 1 second's data loss, it means you need to flush all dirty tables or indexes 
to disk every second. You cannot predict that which page will be modified thus dirty pages 
may be here and there. They are lots of RANDOM writes. Of course, they are very slow and decrease 
throughput.

With redo log, you needn't flush disk every second, just append log to the redo log files and have 
a background worker syncs dirty pages to disk steadily. They are sequence writes. Sequence writes 
are ten times faster than lots of random writes.

#### backup
For very large database, full backup takes too much time and disk space. With redo log, you can 
do a full backup and then backup the redo logs only since it contains all the changes after the 
full backup. This saves time and space.

Since redo log is a sequence of transaction record. Then you can choose to recovery to any point 
in the sequence. This is PITR (point in time recovery). For full backup, you cannot achieve this.

#### replication
Master/slave architecture is often used for high availability. Then how to replicate data from 
master to slaves?

You may think just replay all SQLs in the slaves. It may be acceptable but with lots of limitations.

  * think about update that has random() function, it gave different results at each nodes
  * in concurrent situations, different execute order may lead to different results
  * think about query like `update t1 set status = 1 where id in (select id from t1 limit 10)`,
    query result without order by is uncertain

So statement replication is not a good choice and seldom used.

Then redo log is a nice choice. It is row based and contains changes of each row. So slaves only 
need to replay each row changes to get synced with the master.
