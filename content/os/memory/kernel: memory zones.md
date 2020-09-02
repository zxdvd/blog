```metadata
tags: linux, memory
```

## kernel: memory zones
Kernel divides physical memory to different zones. And each zone may have special usage.

### why zones
Kernel cannot treat each physical page identically due to many situations.

- Some hardware devices only have limited address bits and they can only do DMA at lower
 address space.

- Some architectures support physical memory that larger than virtual address space (
 especially 32 bit OS that virtual memory is 4 GB) so that some memory is not mapped into
 kernel address space permanently.

To avoid this, kernel divides physical memory to zones like ZONE_DMA, ZONE_DMA32, ZONE_NORMAL,
 ZONE_HIGHMEM (not all are used in your system).

For x86 architecture, the lower 16MB (14 bit) is assigned to ZONE_DMA. And for x86-64, the
 rest of the lower 4G (32 bit) is assigned to ZONE_DMA32.

For 32 bit architectures like i386, the kernel address space (1G by default) cannot hold
 all physical memory. Then ZONE_HIGHMEM is used for those cannot be mapped permanently memory.

For x86-64, there isn't ZONE_HIGHMEM since all physical memory are mapped into kernel space.
 And if physical memory is less than 4GB, there won't have ZONE_NORMAL since ZONE_DMA32 is
 enough.

### why ZONE_HIGHMEM
For x86, the virtual memory space is 4GB (32bit). And it is splitted as 3GB userspace and
 1GB kernel space. Each process has a different 3GB virtual memory space and they are isolated.
 During process switching, the page tables also switches.

But the 1GB kernel space is shared. And kernel space doesn't use the page table to do the
 mapping. The 1GB physical memory is mapped into virtual memory space via a simple `offset`.

The offset is defined as `PAGE_OFFSET`, and it is  0XC0000000 (3GiB) in x86. So for an address
 X in the range [0, 1GB), the virtual address is `X + 0XC0000000`. This is very fast accessing.

Then for address that greater than 1GB, the virtual address will be large than 4GB which overflows
 the 32 bit virtual address space. So for kernel, physical address above the lower 1GB cannot
 be accessed directly.

Then kernel names it as ZONE_HIGHMEM and it cannot be used to store kernel objects.

For 64 bit architectures, like x86-64, the virtual memory space is large enough. And it reserves
 a range for whole physical memory. So it will never have the overflow problem. Then it do not
 need the ZONE_HIGHMEM zone.

### why ZONE_HIGHMEM begins from 896MB but not 1GB
From above we may guess that ZONE_HIGHMEM begins from 1GB but actually it begins from 896MB. The
 kernel space has 1GB. And the kernel needs to reserve some space for page tables, IO and others.
 The kernel reserves 128MB from the tail. So that the directly mapped range is not [0, 1GB) but
 [0, 1024MB - 128MB=896MB). So ZONE_HIGHMEM begins from 896MB.

### zone info
You can use `cat /proc/buddyinfo` to get information about buddy system of each zones.

```
❯ cat /proc/buddyinfo
Node 0, zone      DMA      0      0      0      1      2      1      1      0      1      1      3
Node 0, zone    DMA32  28915   3569   3636   2038   1110    376    109      3      1      0      0
Node 0, zone   Normal 899905 319741  63711   5712   4843    126      4      2      0      0      0
```

From left to right, the numbers show available chunks of size 4kb, 8kb, 16kb and so on of each zone.
For above case, the DMA zone has three 4MB chunks available. The Normal zone has 899905 4kb chunks
 available.

You can use `cat /proc/pagetypeinfo` to get more detail information. It shows chunk number of different
 status of each zones.

```
❯ cat /proc/pagetypeinfo
Page block order: 9
Pages per block:  512

Free pages count per migrate type at order       0      1      2      3      4      5      6      7      8      9     10
Node    0, zone      DMA, type    Unmovable      0      0      0      1      2      1      1      0      1      0      0
Node    0, zone      DMA, type      Movable      0      0      0      0      0      0      0      0      0      1      3
Node    0, zone      DMA, type  Reclaimable      0      0      0      0      0      0      0      0      0      0      0
Node    0, zone      DMA, type   HighAtomic      0      0      0      0      0      0      0      0      0      0      0
Node    0, zone      DMA, type          CMA      0      0      0      0      0      0      0      0      0      0      0
Node    0, zone      DMA, type      Isolate      0      0      0      0      0      0      0      0      0      0      0
Node    0, zone    DMA32, type    Unmovable     18     40     44     78     62     52     24      2      0      0      0
Node    0, zone    DMA32, type      Movable  27406   2993   1110    282     12      1      0      0      0      0      0
Node    0, zone    DMA32, type  Reclaimable   1762    533   2473   1656   1020    315     82      0      0      0      0
Node    0, zone    DMA32, type   HighAtomic     27     24     17     22     16      8      3      1      1      0      0
Node    0, zone    DMA32, type          CMA      0      0      0      0      0      0      0      0      0      0      0
Node    0, zone    DMA32, type      Isolate      0      0      0      0      0      0      0      0      0      0      0
Node    0, zone   Normal, type    Unmovable    283   2340   7265   2839    442     21      0      1      0      0      0
Node    0, zone   Normal, type      Movable 890333 303420  19062    248   3683     85      2      0      0      0      0
Node    0, zone   Normal, type  Reclaimable    119  13062  36709   2323    661     11      0      0      0      0      0
Node    0, zone   Normal, type   HighAtomic    963    851    675    298     57      9      2      1      0      0      0
Node    0, zone   Normal, type          CMA      0      0      0      0      0      0      0      0      0      0      0
Node    0, zone   Normal, type      Isolate      0      0      0      0      0      0      0      0      0      0      0

Number of blocks type     Unmovable      Movable  Reclaimable   HighAtomic          CMA      Isolate
Node 0, zone      DMA            1            7            0            0            0            0
Node 0, zone    DMA32           21         1307          197            3            0            0
Node 0, zone   Normal          390        29177         1639           26            0            0
```

### references
- [linux-mm: virtual memory](https://linux-mm.org/VirtualMemory)
- [stackoverflow: why zone normal limited to 896MB](https://stackoverflow.com/questions/8252785/why-linux-kernel-zone-normal-is-limited-to-896-mb)
