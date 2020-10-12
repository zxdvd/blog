```metadata
tags: postgres, sourecode, storage, page, tuple
```

## postgres storage: page and tuple

Each page is 8k (8192) by default. You need to recompile and build from source if you
 want to use a large page size. A page is stored as a buffer when loaded into memory.

Each page has page header and content. For content, it is always divided into two parts:
 item pointers and tuples. Each item pointer is fixed length and points to a tuple. A
 tuple may be a row of table or index.

### tuple and item

`ItemId` is line pointer inside a page. It has offset inside page, flags and byte length
 of tuple.

```c
// src/include/storage/itemid.h
typedef struct ItemIdData
{
	unsigned	lp_off:15,		/* offset to tuple (from start of page) */
				lp_flags:2,		/* state of line pointer, see below */
				lp_len:15;		/* byte length of tuple */
} ItemIdData;

typedef ItemIdData *ItemId;
```

The lg_flags has 4 states: unused, normal, redirect and dead.

`Item` is defined as `typedef Pointer Item`, it holds actually tuple data.


Item pointer needs block number (4 bytes) and offset (2 bytes) in block. It tries to
 padding them to save memory if compilers supported.

```c
// src/include/storage/itemptr.h
typedef struct ItemPointerData
{
	BlockIdData ip_blkid;
	OffsetNumber ip_posid;
}
ItemPointerData;

typedef ItemPointerData *ItemPointer
```


The item pointer offset inside a block is of type uint16. And it counts from 1. 0 is
 used as invalid offset.

```c
// src/include/storage/off.h
typedef uint16 OffsetNumber;

#define InvalidOffsetNumber		((OffsetNumber) 0)
#define FirstOffsetNumber		((OffsetNumber) 1)
```


### page
Page layout:

`pd_lower` is start of free space while `pd_upper` is end of free space.
`linp1` is line pointer 1 (start from 1 but not 0).

But what is `special space`? A page may be used for table rows, btree index,
 hash index and so on. The different types of page need space to store
 metadata info. That's `special space`. And size of `special space` varies
 according types.

```
 * +----------------+---------------------------------+
 * | PageHeaderData | linp1 linp2 linp3 ...           |
 * +-----------+----+---------------------------------+
 * | ... linpN |									  |
 * +-----------+--------------------------------------+
 * |		   ^ pd_lower							  |
 * |												  |
 * |			 v pd_upper							  |
 * +-------------+------------------------------------+
 * |			 | tupleN ...                         |
 * +-------------+------------------+-----------------+
 * |	   ... tuple3 tuple2 tuple1 | "special space" |
 * +--------------------------------+-----------------+
 *									^ pd_special
```

`PageHeaderData` stores checksum, flags, version and offsets.

```c
// src/include/storage/bufpage.h
typedef struct PageHeaderData
{
	/* XXX LSN is member of *any* block, not only page-organized ones */
	PageXLogRecPtr pd_lsn;		/* LSN: next byte after last byte of xlog
								 * record for last change to this page */
	uint16		pd_checksum;	/* checksum */
	uint16		pd_flags;		/* flag bits, see below */
	LocationIndex pd_lower;		/* offset to start of free space */
	LocationIndex pd_upper;		/* offset to end of free space */
	LocationIndex pd_special;	/* offset to start of special space */
	uint16		pd_pagesize_version;
	TransactionId pd_prune_xid; /* oldest prunable XID, or zero if none */
	ItemIdData	pd_linp[FLEXIBLE_ARRAY_MEMBER]; /* line pointer array */
} PageHeaderData;

typedef PageHeaderData *PageHeader;
```

`PageInit` initializes a page.

```c
void
PageInit(Page page, Size pageSize, Size specialSize)
{
	PageHeader	p = (PageHeader) page;

	specialSize = MAXALIGN(specialSize);

	Assert(pageSize == BLCKSZ);
	Assert(pageSize > specialSize + SizeOfPageHeaderData);

	/* Make sure all fields of page are zero, as well as unused space */
	MemSet(p, 0, pageSize);

	p->pd_flags = 0;
	p->pd_lower = SizeOfPageHeaderData;
	p->pd_upper = pageSize - specialSize;
	p->pd_special = pageSize - specialSize;
	PageSetPageSizeAndVersion(page, pageSize, PG_PAGE_LAYOUT_VERSION);
	/* p->pd_prune_xid = InvalidTransactionId;		done by above MemSet */
}
```

The `PageAddItemExtended` is used to overwrite an existed item or find a new unused item.

```c
OffsetNumber
PageAddItemExtended(Page page,
					Item item,
					Size size,
					OffsetNumber offsetNumber,
					int flags)
{
}
```


### block
A block is a disk block. Each data file (table, index, fsm) are divided into many blocks.
A block is mapped to a buffer if loaded.

`BlockNumber` is a uint32.

```c
// src/include/storage/block.h
typedef uint32 BlockNumber;

typedef struct BlockIdData
{
	uint16		bi_hi;
	uint16		bi_lo;
} BlockIdData;

typedef BlockIdData *BlockId;	/* block identifier */
```

### references
