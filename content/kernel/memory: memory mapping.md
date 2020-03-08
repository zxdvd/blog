<!---
tags: linux, memory, mmap
-->

## memory mapping
A memory mapping is a set of page table entries describing the properties of a
 consecutive virtual address range. Each memory mapping has a start address,
 length and permissions (RWX).

Create a memory mapping only allocates virtual memory, not physical memory (
 except small physical memory to store metadata info). Physical memory is
 allocated by page fault when first writing a page of virtual memory.

### anonymous memory mapping
An anonymous memory mapping is a memory mapping that has no file or device backing
 it. Stack and heap use it underground.

Anonymous mapping won't allocate physical memory. When reading, a copy on write
 zerod page are returned. And write to page will trigger page fault handler 
 that will do the physical memory allocation.

### mmap vs read
You can open and read it to process a file. This is the simplest way. You can
 also use mmap to map the file into the virtual address directly.

It may or may not improve the performance. It depends. The mmap way has following
 advantages:

    - no system call after mmap since the file is in the address space while read is 
      a system call that you may need many context switches during read while file
    - no copy from kernel space to user space (read will copy)
    - if you need to read multiple times, mmap may save a lot system calls and copies

However, mmap also has some disadvantages:

    - complex than read; everybody knows about open/read/write
    - mmap itself has more overhead than read (setup and teardown)
    - too many page entries setup, TLB update, too many page faults; with read, only
      a small buffer is needed and cycled

A very nice post about this at
[stackoverflow](https://stackoverflow.com/questions/45972/mmap-vs-reading-blocks/41419353#41419353).
