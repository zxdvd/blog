<!---
tags: redis, ziplist
-->

Ziplist is similar to zipmap with following differences:

- It uses 2 bytes to stores total length, avoid performance problem with zipmap more than 253 entries
- It is a double linked list so that you can traverse from both ends
- It has two encoding types: string and integer and it can be very efficient to store integer, especially small integers

### memory layout
Following is the memory layout for ziplist. The 4 bytes `zlbytes` is total number of
 bytes that including itself.

The `zltail` is offset to the last entry from beginning of the ziplist so that you can
 just to it quickly and traverse from end to head.

For `zllen`, if you have more than 2^16-2, then just like zipmap, you need to traverse
 whole ziplist to get the exactly length. Luckily, 2^16-2 is enough in most cases.

The `zlend` is fixed 255 (0xFF) that indicates end. So when you get a 0xFF byte after
 a entry, you've got the end. Don't worry about the case that the entry has a 0xFF as
 beginning ocassionally. You won't get this and I'll explain this later.

```
  uint32   uint32  uint16                               fixed 255
<zlbytes> <zltail> <zllen> <entry> <entry> ... <entry> <zlend>
```

For each entry in ziplist, it may have 3 or 2 parts:

```
<prevlen> <encoding> <entry-data>
<prevlen> <encoding>
```

The `prevlen` is length of previous entry so that we can just to previous one. It will
 be one byte is less or equal to 253. Otherwise, it will be 5 bytes.
First byte is 254 (0xFE) and the next 4 bytes is the length. Then we can see that the
 first byte of a entry will never be 255. So if you got 255, it is the end.

The `encoding` is a little complicated. Its length is not fixed. Following is comment
 from source code:

It's integer if first two bits of encoding is `11`. Otherwise, it's string.
For integer between 0 to 12. The 1 byte encoding is enough to hold it. And this is the
 case that an entry has only `prevlen` and `encoding`.

```
 * |00pppppp| - 1 byte
 *      String value with length less than or equal to 63 bytes (6 bits).
 *      "pppppp" represents the unsigned 6 bit length.
 * |01pppppp|qqqqqqqq| - 2 bytes
 *      String value with length less than or equal to 16383 bytes (14 bits).
 *      IMPORTANT: The 14 bit number is stored in big endian.
 * |10000000|qqqqqqqq|rrrrrrrr|ssssssss|tttttttt| - 5 bytes
 *      String value with length greater than or equal to 16384 bytes.
 *      Only the 4 bytes following the first byte represents the length
 *      up to 32^2-1. The 6 lower bits of the first byte are not used and
 *      are set to zero.
 *      IMPORTANT: The 32 bit number is stored in big endian.
 * |11000000| - 3 bytes
 *      Integer encoded as int16_t (2 bytes).
 * |11010000| - 5 bytes
 *      Integer encoded as int32_t (4 bytes).
 * |11100000| - 9 bytes
 *      Integer encoded as int64_t (8 bytes).
 * |11110000| - 4 bytes
 *      Integer encoded as 24 bit signed (3 bytes).
 * |11111110| - 2 bytes
 *      Integer encoded as 8 bit signed (1 byte).
 * |1111xxxx| - (with xxxx between 0000 and 1101) immediate 4 bit integer.
 *      Unsigned integer from 0 to 12. The encoded value is actually from
 *      1 to 13 because 0000 and 1111 can not be used, so 1 should be
 *      subtracted from the encoded 4 bit value to obtain the right value.
 * |11111111| - End of ziplist special entry.
```

### ziplist usage
Currently ziplist is used in hash and zset when entry count and value is small. The
 ziplist hash will be converted to dict when it becomes too large. The ziplist zset
 will be converted to skiplist.

It used to be used for small list but then been replaced by quicklist which uses
 ziplist underground.

Redis provides settings to control when they will be converted.

Following is the default settings:

```
# Hashes are encoded using a memory efficient data structure when they have a
# small number of entries, and the biggest entry does not exceed a given
# threshold. These thresholds can be configured using the following directives.
hash-max-ziplist-entries 512
hash-max-ziplist-value 64

zset-max-ziplist-entries 128
zset-max-ziplist-value 64
```

### References:
1. [github: redis ziplist](https://github.com/antirez/redis/blob/unstable/src/ziplist.c)
