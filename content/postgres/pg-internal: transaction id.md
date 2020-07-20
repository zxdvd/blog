```metadata
tags: postgres, internal, transaction
```

## pg-internal: transaction id
Each transaction has transaction id. The full transaction id is composed of two parts:
 an uint32 epoch and an uint32 transaction id. Considering that database usually runs
 for very long time, the uint32 transaction id may be used out quickly. Then postgres
 will do a `epoch++` and reset transaction id from beginning.

```c
// postgres: src/include/access/transam.h

#define EpochFromFullTransactionId(x)	((uint32) ((x).value >> 32))
#define XidFromFullTransactionId(x)		((uint32) (x).value)

typedef struct FullTransactionId
{
	uint64		value;
} FullTransactionId;
```

However, most of time, postgres only use the 32bit transaction id. For example, the xmin
 and xmax of each row and the clog only store the 32bit transaction id. Then it leads to
 a problem that how did it know whether transaction id 10 is greater than `2<<32-1` or not
 without the epoch?

Postgres avoids this by setting a hard limit on all visible transaction ids. Suppose N is
 the smallest transaction of the cluster, then the greatest transaction id is limit to
 `(N + 2<<31)` (result will modulo to fix uint32). So when you see transaction id 10 and
 `2<<32-1` at same time, then we can sure that transaction wrapping happens and transaction
 id 10 is bigger.

#### what will happen if we reach the N+2<<31 limit?
This will rarely happen since N is dynamic. The autovacuum process will freeze old enough
 rows. They are unlikely changed since they havn't been changed for a long time. The transaction
 id of frozen rows are changed to a special value 2. So that N becomes large as small
 transactions are frozen.

Postgres has three watermark about this.

```c
// postgres: src/backend/access/transam/varsup.c

void SetTransactionIdLimit(TransactionId oldest_datfrozenxid, Oid oldest_datoid) {
    ...
    xidWrapLimit = oldest_datfrozenxid + (MaxTransactionId >> 1);
    xidStopLimit = xidWrapLimit - 1000000;
    xidWarnLimit = xidStopLimit - 10000000;
    xidVacLimit = oldest_datfrozenxid + autovacuum_freeze_max_age;
    ...
}
```

The `xidVacLimit` is a point that the master may trigger the autovacuum to start cleaning.
And the `xidWarnLimit` is a point that it will warn you that only 10 million transactions
 are left to use.

The `xidStopLimit` is a point that only 1 million is left and it will refuse to accept
 commands and you should stop the database and start with single user mode and do vacuum
 manually. Attention, vaccum also needs one transaction for each table.

The code of this logic is at function `FullTransactionId GetNewTransactionId(bool isSubXact)`
 of `varsup.c`.

### transaction id assignment
At beginning I said `Each transaction has transaction id`. It may not true sometimes.
 Actually, the transaction tries to get transaction id as late as possible. For `SELECT`
 query, it won't get transaction id. You'll get transaction id when you need to modify
 data (insert, update, delete).

For example, the `heap_update()` will be executed when you update a row.

```c
// postgres: src/backend/access/heap/heapam.c

TM_Result
heap_update(Relation relation, ItemPointer otid, HeapTuple newtup,
			CommandId cid, Snapshot crosscheck, bool wait,
			TM_FailureData *tmfd, LockTupleMode *lockmode) {

	TransactionId xid = GetCurrentTransactionId();
    ...
}
```

The `GetCurrentTransactionId` will try to get transaction id from transaction state.
It will call `AssignTransactionId(s)` if the transaction id is invalid (it is initiated
 as `InvalidTransactionId`).

```c
// postgres: src/backend/access/transam/xact.c

TransactionId GetCurrentTransactionId(void) {
	TransactionState s = CurrentTransactionState;

	if (!FullTransactionIdIsValid(s->fullTransactionId))
		AssignTransactionId(s);
	return XidFromFullTransactionId(s->fullTransactionId);
}
```

You can find `GetCurrentTransactionId()` in other heap functions that will modify data,
 like `heap_insert`, `heap_multi_insert`, `heap_delete`. But you won't get it in
 `heap_fetch`.
