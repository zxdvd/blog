<!---
tags: postgres, internal, transaction, snapshot, isolation
-->

## pg-internal: transaction snapshot
Transaction snpashot records state of the transaction. Low level access api needs it to
 check the visibility of a tuple.

Two key snapshot related source code files are:

    src/include/utils/snpashot.h
    src/backend/utils/time/snapmgr.c

### Snapshot definition
Following is definition of snapshot data structure in `snapshot.h` (not all fields
 is copied).

There are several snapshot types. But the most common one is the `SNAPSHOT_MVCC`.
It is used in all general sql queries. The `SNAPSHOT_ANY` is easy to understand:
 read all tuples no matter it is deleted, committed or rollbacked. The `pg_dirtyread`
 extension uses this type to read all existed tuples. It seems that the `SNAPSHOT_SELF`
 is not used. For other types, you can read the comments in the definition.

For the MVCC snapshot, the `xmin` is the lower boundary of visible transactions. All
 transaction below it is visible. But you still needs to check whether it is committed
 or rollbacked. All transaction id that equal or large than `xmax` is invisible.

The `xip[]` is a collection of active transactions.

The `curcid` is sequence of the current command in this transaction. So any `CID` less
 than it is also visible.

```c
typedef struct SnapshotData
{
	SnapshotType snapshot_type; /* type of snapshot */

	/*
	 * The remaining fields are used only for MVCC snapshots, and are normally
	 * just zeroes in special snapshots.  (But xmin and xmax are used
	 * specially by HeapTupleSatisfiesDirty, and xmin is used specially by
	 * HeapTupleSatisfiesNonVacuumable.)
	 *
	 * An MVCC snapshot can never see the effects of XIDs >= xmax. It can see
	 * the effects of all older XIDs except those listed in the snapshot. xmin
	 * is stored as an optimization to avoid needing to search the XID arrays
	 * for most tuples.
	 */
	TransactionId xmin;			/* all XID < xmin are visible to me */
	TransactionId xmax;			/* all XID >= xmax are invisible to me */

	/*
	 * note: all ids in xip[] satisfy xmin <= xip[i] < xmax
	 */
	TransactionId *xip;
	uint32		xcnt;			/* # of xact ids in xip[] */
	CommandId	curcid;			/* in my xact, CID < curcid are visible */
	TimestampTz whenTaken;		/* timestamp when snapshot was taken */
	XLogRecPtr	lsn;			/* position in the WAL stream when taken */
} SnapshotData;
```

### get snapshot, isolation level
The `GetTransactionSnapshot` function in `snapmgr.c` is used to get a snapshot. You
 need a snapshot before accessing data.

We know that transaction has different isolation levels: read committed, repeatable
 read, serializable. This function deals with this.

It uses a global `FirstSnapshotSet` variable to record whether the snapshot was set
 in this transaction. For repeatable read and serializable isolation level, it will
 get a new snapshot at the beginning and always return this first snapshot in this
 transaction later. So that each commands in this transaction get a same consistent
 snapshot of the beginning moment.


However, for the `read committed` isolation level, it will use the `GetSnapshotData`
 function in `procarray.c` to update `xmin`, `xmax` and `xip` each time it was called.

```c
	CurrentSnapshot = GetSnapshotData(&CurrentSnapshotData);
```

 So in a transaction, if you have command1, command2 to be executed, command2 can
 see tuples that committed after beginning of the transaction but before executing of
 command2. That's why it is called `read committed`.

