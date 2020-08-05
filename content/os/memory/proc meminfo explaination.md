```metadata
tags: linux, memory, proc
```

## /proc/meminfo explaination

The /proc/meminfo can give you more detailed information about system memory usage.
But what do those fields mean?

I dumped following content from a production machine (kernel version 5.5.4).

```
# cat /proc/meminfo
MemTotal:       16400112 kB
MemFree:          532460 kB
MemAvailable:    9668832 kB
Buffers:         1036504 kB
Cached:          7061516 kB
SwapCached:            0 kB
Active:          9236936 kB
Inactive:        5101656 kB
Active(anon):    5807496 kB
Inactive(anon):    39052 kB
Active(file):    3429440 kB
Inactive(file):  5062604 kB
Unevictable:          48 kB
Mlocked:              48 kB
SwapTotal:             0 kB
SwapFree:              0 kB
Dirty:              3052 kB
Writeback:             0 kB
AnonPages:       5770784 kB
Mapped:           725452 kB
Shmem:             71228 kB
KReclaimable:     982004 kB
Slab:            1322496 kB
SReclaimable:     982004 kB
SUnreclaim:       340492 kB
KernelStack:       31968 kB
PageTables:        65696 kB
NFS_Unstable:          0 kB
Bounce:                0 kB
WritebackTmp:          0 kB
CommitLimit:     8200056 kB
Committed_AS:   19899264 kB
VmallocTotal:   34359738367 kB
VmallocUsed:       41164 kB
VmallocChunk:          0 kB
Percpu:             7856 kB
HardwareCorrupted:     0 kB
AnonHugePages:   1355776 kB
ShmemHugePages:        0 kB
ShmemPmdMapped:        0 kB
FileHugePages:         0 kB
FilePmdMapped:         0 kB
HugePages_Total:       0
HugePages_Free:        0
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB
Hugetlb:               0 kB
DirectMap4k:      440192 kB
DirectMap2M:    15288320 kB
DirectMap1G:     3145728 kB
```

You can check source code which generates the `/proc/meminfo` at `fs/proc/meminfo.c` of
 the kernel. The function `meminfo_proc_show` will generate above content. Follow the
 code to get source of each field.

### buffers and cached
The `si_meminfo` in `mm/page_alloc.c` is used to get some fields include the `buffers`.
And it calls the `nr_blockdev_pages()` to get buffers.

```c
long nr_blockdev_pages(void)
{
	struct block_device *bdev;
	long ret = 0;
	spin_lock(&bdev_lock);
	list_for_each_entry(bdev, &all_bdevs, bd_list) {
		ret += bdev->bd_inode->i_mapping->nrpages;
	}
	spin_unlock(&bdev_lock);
	return ret;
}
```

From above code, we know that it just sum cached pages of each block devices. So the
 `buffers` is memory used by cache of block devices.

```c
// kernel: fs/proc/meminfo.c

cached = global_node_page_state(NR_FILE_PAGES) -
			total_swapcache_pages() - i.bufferram;
```

It's all mapped files in memory (exclude swapped) with pages of block devices excluded.
So `buffers + cached` is in memory usage of mapped files.

### active and inactive
Linux caches a lot of used files so that operations on them are very fast. However, it
 needs to evict some if system doesn't have enough free memory. So which pages to evict?

Linux splits pages to active and inactive. The active pages are those been used and cannot
 be evicted. And inactive are those can be evicted safely. Pages may be moved between
 them due to different situations.

### anoymous and file backed memory
A page can be file backed or anoymous. If you `mmap` a file into memory, then the allocated
 pages are file backed. If you use `malloc` (this uses anoymous `mmap` internally) to get
 memory , then you got anoymous pages.

Each of the two types are splitted to active and inactive. Then it comes following:

	Active = Active(anon) + Active(file)
	Inactive = Inactive(anon) + Inactive(file)

	AnonPages:       5770784 kB
	Mapped:           725452 kB

The `AnonPages` and `Mapped` are read from global counters: `NR_ANON_MAPPED` and `NR_FILE_MAPPED`.
From the name NR_XXX_MAPPED, we know that only mapped pages are counted. Unmapped pages may
 still in inactive LRU for a while.

### unevictable and mlocked
You can use `mlock` to lock a range of virtual memory in RAM so that it will never be swapped.
Swapped content may be read by other process. You can lock pages in memory to avoid this.
You may have confidential information like tokens, private keys and you can use `mlock` to
 lock them so that it won't be stolen by other processes.

All mlocked memory are also unevictable. But I am not sure whether unevictable is also mlocked
 though I found they are equal on my machines.

### swap
You can use swap as supplement of memory. Some inactive pages can be swapped to free up memory.
 But it will be very slow if then swapped pages are needed again.
Swap is disabled in my machine so that related fields are 0.


### dirty and writeback
File backed pages that were changed are marked as dirty. OS will flush them to disk asynchronous.
The `writeback` field is that is in the flush queue and will be write to disk soon.

### shared memory
In linux, shared memory is implemented based on ramfs. See following comments:

```c
// kernel: mm/shmem.c

#ifdef CONFIG_SHMEM
/*
 * This virtual memory filesystem is heavily based on the ramfs. It
 * extends ramfs by the ability to use swap and honor resource limits
 * which makes it a completely usable filesystem.
 */
```

Since it is file system based, shared memory is counted to page cache. And the code also shows
 this:

```c
// kernel: mm/shmem.c

static int shmem_add_to_page_cache(struct page *page,
				   struct address_space *mapping,
				   pgoff_t index, void *expected, gfp_t gfp,
				   struct mm_struct *charge_mm)
{
    ...
		__mod_lruvec_page_state(page, NR_FILE_PAGES, nr);
		__mod_lruvec_page_state(page, NR_SHMEM, nr);
    ...
}
```

The counter counts `NR_SHMEM` (shared memory) and `NR_FILE_PAGES` (cached memory) at the
 same time. Similar code appears at the `shmem_delete_from_page_cache` function.

### slab
The kernel also needs to malloc and free some data structures just like normal applications.
The slab allocator is used for this situation.

	Slab:            1322496 kB
	SReclaimable:     982004 kB
	SUnreclaim:       340492 kB

The `SReclaimable` and `SUnreclaim` come from global state `NR_SLAB_RECLAIMABLE` and
 `NR_SLAB_UNRECLAIMABLE`. The `Slab` field is just sum of the two.

### kernel and kernel reclaimable
Each task (thread) has a kernel stack, 16kb on x86-64 by default (THREAD_SIZE). So this
 machine has 31968/16=1998 tasks.

The `KReclaimable` is sum of `SReclaimable` and total size of `NR_KERNEL_MISC_RECLAIMABLE`.
 On my machine, `KReclaimable` equals to `SReclaimable`.

    KReclaimable:     982004 kB
    KernelStack:       31968 kB

### page tables
Each process has isolated virtual memory space. Then each process has its own page tables
 to map virtual address and physical address. The `PageTables` is total memory of these
 page tables.

    PageTables:        65696 kB

### CommitLimit and Committed_AS
`CommitLimit` field limits the virtual address size that a process created. You can use
 `/proc/sys/vm/overcommit_kbytes` or `/proc/sys/vm/overcommit_ratio` to tune this. By
 default, it is 50% of `MemTotal`. `Committed_AS` is the committed size.

	CommitLimit:     8200056 kB
	Committed_AS:   19899264 kB

### vmalloc
VmallocTotal is total size of of vmalloc range, it is static compiled and varies depending
 on architectures. VmallocUsed is used size. VmallocChunk is always 0 currently.

    VmallocTotal:   34359738367 kB
    VmallocUsed:       41164 kB
    VmallocChunk:          0 kB

### percpu
Memory allocated by percpu allocator.

    Percpu:             7856 kB

### hugepage
For machine supports hugepage, you can enable it and split some memory to it. Use hugepage
 can save a lot of page table entries and page structs. The bad side is that, this splitted
 memory can only be used by hugepage. You cannot change the size dymanically.

I didn't use huge page in this machine so all related fields are 0.

### direct map
The direct map entries represent how many pages of 4k, 2M, 1G are mapped in TLB.
