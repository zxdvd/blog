```metadata
tags: linux, memory
```

## memory: from free to meminfo
It's very common to use `free` to get memory usage of system.

```
❯ free -m
              total        used        free      shared  buff/cache   available
Mem:          64429       12329       18055         793       34043       50630
Swap:             0           0           0
```

So what does it mean and where does free get it from? After reading the source code of
 free, I found that free get the result from `/proc/meminfo`.

### memory fields

The total memory field comes from `MemTotal` of `/proc/meminfo`. And the free field comes
 from `MemFree`.

The used field is calculated as following:

```c
mem_used = kb_main_total - kb_main_free - kb_main_cached - kb_main_buffers;
  if (mem_used < 0)
    mem_used = kb_main_total - kb_main_free;
  kb_main_used = (unsigned long)mem_used;
```

So we can find that `buff/cache` is not included in used memory if calculated result is
 positive.

The shared field comes from `Shmem`.

The `buff/cache` is sum of `buffers` and `cache`. The buffers field comes from `Buffers`.

The `cache` is also calculated. It is sum of `Cached` field and `SReclaimable`.

```c
  kb_main_cached = kb_page_cache + kb_slab_reclaimable;
```

Available memory is calculated as following and it will fallback to kb_main_free if the
 result is negative or large than total memory.

```c
      watermark_low = kb_min_free * 5 / 4; /* should be equal to sum of all 'low' fields in /proc/zoneinfo */
      mem_available = (signed long)kb_main_free - watermark_low
      + kb_inactive_file + kb_active_file - MIN((kb_inactive_file + kb_active_file) / 2, watermark_low)
      + kb_slab_reclaimable - MIN(kb_slab_reclaimable / 2, watermark_low);
```

The `kb_min_free` is value of `/proc/sys/vm/min_free_kbytes`. So what does it mean?

You'll get trouble if the kernel use out all memory. Kernel needs to reserve a small number
 of memory to keep the kernel running without problems. The `/proc/sys/vm/min_free_kbytes`
 is used to tune this value. You'd better not set this lower than 1024.

The `kb_inactive_file` comes from field `Inactive(file)` while the `kb_active_file` comes
 from `Active(file)`.

### swap fields
The swap total comes from `SwapTotal` while the swap free comes from `SwapFree`. Swap used
 is minus of the two.

From above we know free just extracts information from `/proc/meminfo` and gives us a simple
 and friendly output. You can go to `/proc/meminfo` to get detailed information about memory.

### references
- [free source code: proc/sysinfo.c](https://gitlab.com/procps-ng/procps/-/blob/master/proc/sysinfo.c)
- [kernel doc: sysctl vm](https://www.kernel.org/doc/Documentation/sysctl/vm.txt)
