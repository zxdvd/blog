```metadata
tags: postgres, sourecode, storage, buffer
```

## postgres storage: buffer
Buffer is in memory mapping of a disk block. The `Buffer` type is defined simply as `int`
 in postgres. 0 means invalid buffer, positive index means shared buffer and negative
 index means local buffer.

### shared buffers
The shared buffers are used by all processes among the whole cluster. Postgres uses it
 to store hot data (tables, indexes and some other) in memory to avoid read disk.

It is suggested to set shared buffers as 25% of total memory. And you cannot change it
 when the cluster is running since the buffers are fixed allocated at start time.

### local buffers
Each backend may allocate local buffers for temporary tables since temporary tables are
 only visible to same session (only the owned process) and also not logged (so not crash
 safe). So postgres uses local buffers for this to avoid lock and WAL log.

The config for local buffers size is `temp_buffers`. It is 8MB by default but you can
 change it at begin of each session (before first use of temporary tables).

### buffer tag
A buffer tag specifies the disk block of the buffer. It contains relation identifier,
 fork number and block number.

A relation may have many forks: main fork (table or index), FSM fork (free space) and so
 on.

```c
// src/include/storage/buf_internals.h

typedef struct buftag
{
	RelFileNode rnode;			/* physical relation identifier */
	ForkNumber	forkNum;
	BlockNumber blockNum;		/* blknum relative to begin of reln */
} BufferTag;
```

### buffer description
`BufferDesc` is description of a buffer.

Postgres stores all `BufferDesc` in an array of shared memory. And they may be used by
 multiple processes with high frequency. So postgres pads it to 64 bytes on 64 bit
 machine to match cache line size. This can avoid false sharing.

`BufferDescriptors` is the array for shared buffers while `LocalBufferDescriptors` is
 for local buffers (it is not padded since it is used by only one process).

```c
// src/include/storage/buf_internals.h

typedef struct BufferDesc
{
	BufferTag	tag;			/* ID of page contained in buffer */
	int			buf_id;			/* buffer's index number (from 0) */

	/* state of the tag, containing flags, refcount and usagecount */
	pg_atomic_uint32 state;

	int			wait_backend_pid;	/* backend PID of pin-count waiter */
	int			freeNext;		/* link in freelist chain */

	LWLock		content_lock;	/* to lock access to buffer contents */
} BufferDesc;


#define BUFFERDESC_PAD_TO_SIZE	(SIZEOF_VOID_P == 8 ? 64 : 1)

typedef union BufferDescPadded
{
	BufferDesc	bufferdesc;
	char		pad[BUFFERDESC_PAD_TO_SIZE];
} BufferDescPadded;

extern PGDLLIMPORT BufferDescPadded *BufferDescriptors;
extern BufferDesc *LocalBufferDescriptors;
```

The `state` is a combination of flags, refcount and usagecount. It is defined as atomic
 int and postgres uses `CAS` to lock and change the buffer description.

```c
// src/include/storage/buf_internals.h
#define BM_LOCKED				(1U << 22)	/* buffer header is locked */
#define BM_DIRTY				(1U << 23)	/* data needs writing */
#define BM_VALID				(1U << 24)	/* data is valid */
#define BM_TAG_VALID			(1U << 25)	/* tag is assigned */
#define BM_IO_IN_PROGRESS		(1U << 26)	/* read or write in progress */
#define BM_IO_ERROR				(1U << 27)	/* previous I/O failed */
#define BM_JUST_DIRTIED			(1U << 28)	/* dirtied since write started */
#define BM_PIN_COUNT_WAITER		(1U << 29)	/* have waiter for sole pin */
#define BM_CHECKPOINT_NEEDED	(1U << 30)	/* must write for checkpoint */
#define BM_PERMANENT			(1U << 31)	/* permanent buffer (not unlogged,
											 * or init fork) */
```

### buffer: private refcount
Each buffer has refcount to record how much times it is referenced. You cannot invalid
 a buffer if it is referenced. A process may reference same buffer multiple times. To
 save concurrently updating of buffers, Each process has a private (not shared) entry set
 to store buffer refcount. It is used to be an array which size is `NBuffers`.

But for server with large amount of shared buffer, it leads to two problems:

- too much memory usage (allocate a large array but only few elements are used)
- random access of large array is not cache friendly, too much cache miss

Then a small fixed array (cache friendly) and hash table (groups on demand) are used to
 replace the origin `NBuffers` array.

Related discussion is at [here](https://www.postgresql.org/message-id/flat/20140321182231.GA17111%40alap3.anarazel.de).

```c
// src/backend/storage/buffer/bufmgr.c
typedef struct PrivateRefCountEntry
{
	Buffer		buffer;
	int32		refcount;
} PrivateRefCountEntry;

/* 64 bytes, about the size of a cache line on common systems */
#define REFCOUNT_ARRAY_ENTRIES 8

static struct PrivateRefCountEntry PrivateRefCountArray[REFCOUNT_ARRAY_ENTRIES];
static HTAB *PrivateRefCountHash = NULL;
```


### buffer lock
Shared buffers are shared between all backends. Process needs to lock the buffer before
 use it. Postgres used to use a single `LWLock`. From postgres 8.2 (commit 10b9ca3), it
 divided buffers to 128 partitions by default, each uses a `LWLock`, this reduces lock
 contention largely.

### references
