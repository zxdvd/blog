<!---
tags: redis, zipmap
-->

## redis: zipmap
Zipmap is not used from version 2.9.2 (commit ebd85e). It was used for small hashes.
Then it was replaced by ziplist.

When the hash becomes large or it contains large value, then it will be converted
 to a real hash (dict). The two related config are

- hash_max_zipmap_entries
- hash_max_zipmap_value

```c
/* Memory layout of a zipmap, for the map "foo" => "bar", "hello" => "world":
 *
 * <zmlen><len>"foo"<len><free>"bar"<len>"hello"<len><free>"world"
 *
 * <zmlen> is 1 byte length that holds the current size of the zipmap.
 * When the zipmap length is greater than or equal to 254, this value
 * is not used and the zipmap needs to be traversed to find out the length.
 *
 * <len> is the length of the following string (key or value).
 * <len> lengths are encoded in a single value or in a 5 bytes value.
 * If the first byte value (as an unsigned 8 bit value) is between 0 and
 * 253, it's a single-byte length. If it is 254 then a four bytes unsigned
 * integer follows (in the host byte ordering). A value of 255 is used to
 * signal the end of the hash.
 *
 * <free> is the number of free unused bytes after the string, resulting
 * from modification of values associated to a key. For instance if "foo"
 * is set to "bar", and later "foo" will be set to "hi", it will have a
 * free byte to use if the value will enlarge again later, or even in
 * order to add a key/value pair if it fits.
 *
 * <free> is always an unsigned 8 bit number, because if after an
 * update operation there are more than a few free bytes, the zipmap will be
 * reallocated to make sure it is as small as possible.
 *
 * The most compact representation of the above two elements hash is actually:
 *
 * "\x02\x03foo\x03\x00bar\x05hello\x05\x00world\xff"
```

### why switch to ziplist
To get the hash length, it needs to traverse the whole zipmap if length is greater
 than 253. This uses too much CPU and leads to performance problems.

And also the ziplist is more efficient to store integer values.

Related issues:

- [github: issue 188 zmlen is too short](https://github.com/antirez/redis/issues/188)
- [github: issue 285 encode small hashes with ziplist](https://github.com/antirez/redis/issues/285)

### References:
- [github: redis zipmap](https://github.com/antirez/redis/blob/unstable/src/zipmap.c)
