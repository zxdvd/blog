```metadata
tags: postgres, sourecode, storage, buffer
```

## storage: ring buffer
Postgres uses shared buffers to cache disk pages. But size of shared buffers is limit.
 Then postgres uses a simple clock sweep algorithm to evict buffers. This leads to a
 problem: a simple sequence scan of a relation large than shared buffer will evit all
 shared buffers.

To avoid this problem, postgres uses a separate ring buffer for operations like sequence
 scan, vacuum, bulk write.

The ring buffer is defined as following.

```c
// src/backend/storage/buffer/freelist.c
typedef struct BufferAccessStrategyData
{
	/* Overall strategy type */
	BufferAccessStrategyType btype;
	/* Number of elements in buffers[] array */
	int			ring_size;
	int			current;

	/*
	 * True if the buffer just returned by StrategyGetBuffer had been in the
	 * ring already.
	 */
	bool		current_was_in_ring;

	Buffer		buffers[FLEXIBLE_ARRAY_MEMBER];
}			BufferAccessStrategyData;
```

Currently, 4 access strategies defined:

```c
// src/include/storage/bufmgr.h
typedef enum BufferAccessStrategyType
{
	BAS_NORMAL,					/* Normal random access */
	BAS_BULKREAD,				/* Large read-only scan (hint bit updates are
								 * ok) */
	BAS_BULKWRITE,				/* Large multi-block write (e.g. COPY IN) */
	BAS_VACUUM					/* VACUUM */
} BufferAccessStrategyType;
```

Postgres assign different ring buffer size for different access strategy type.
For sequence scan and vacuum, it is 256KB. For bulk write (COPY IN and CREATE TABLE AS)
 it is 16MB. And it is limited to 1/8 of shared buffer size.

The BAS_NORMAL is for normal random access, it means no ring buffer is used. Actually
 it is defined and not used.

```c
// src/backend/storage/buffer/freelist.c
    switch (btype)
        {
            case BAS_NORMAL:
                /* if someone asks for NORMAL, just give 'em a "default" object */
                return NULL;

            case BAS_BULKREAD:
                ring_size = 256 * 1024 / BLCKSZ;
                break;
            case BAS_BULKWRITE:
                ring_size = 16 * 1024 * 1024 / BLCKSZ;
                break;
            case BAS_VACUUM:
                ring_size = 256 * 1024 / BLCKSZ;
                break;

            default:
                elog(ERROR, "unrecognized buffer access strategy: %d",
                     (int) btype);
                return NULL;		/* keep compiler quiet */
        }

        /* Make sure ring isn't an undue fraction of shared buffers */
        ring_size = Min(NBuffers / 8, ring_size);
```

The ring buffer is a internal strategy of shared buffer. It is not exposed to outside.
 High level storage api just calls `StrategyGetBuffer`.

```c
// src/backend/storage/buffer/freelist.c
BufferDesc *
StrategyGetBuffer(BufferAccessStrategy strategy, uint32 *buf_state)
{
	BufferDesc *buf;

	if (strategy != NULL)
	{
		buf = GetBufferFromRing(strategy, buf_state);
		if (buf != NULL)
			return buf;
	}

    // try with freelist
}
```

### references
- [postgres source code: buffer ring replacement strategy](https://github.com/postgres/postgres/blob/master/src/backend/storage/buffer/README#L208)
- [postgres source code: buffer ring](https://github.com/postgres/postgres/blob/master/src/backend/storage/buffer/freelist.c)
