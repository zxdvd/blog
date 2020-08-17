```metadata
tags: data-structure, hash table
```

## hash table

Hash table is very common data structure. And most languages use it to implement data
 containers like `Map`, `Dict` and `Set`.

### advantages
Hash table has following advantages:

- O(1) for get, set, delete by key and add
- key is unique

You can use other data structures like array or linked list to store key value pairs.
But operations like get, set and delete by key are O(n) since you need to iterate the
 whole array or list to get specific key.

And to keep the key unique you need to search the key before add it. So it's also O(n).

So how does hash table achieve O(1) for get, set, add and delete?

### implementation
We know that search an element in array is O(n) since it needs to iterate the while array.
 But access via index is O(1) since address if the Mth element is `HEAD_ADDR + M * sizeof(element)`.

If we can convert search by key to search by index, then it's O(1). The hash table uses
 an array undergound. It hashes the keys to integer and then does modular calculation against
 array length to get an array index. Then all search by key is similar to search by index,
 thus O(1).

However, different keys may hash to same value and it's very common that different values got
 same result after modular calculation. So how to deal with this?

There are two common hash table implementations and they have different strategies.

The `chained hash table` may use a linked list to store key value pairs that falling into same
 bucket. When search by key, you first hash the key to get the bucket index, then loop the
 linked list and compare the key one by one to find the matched one.

Some languages, Java for example, use a red black tree when the linked list is large than a
 threshold.

To deal with the hash conflict, the `open address hash table` will do some calculation again
 and again to find an unused bucket. The calculation varies according specific implementations.
Some may simply try next bucket. Some may hash previous hash again.

For the open address hash table, you need to deal with deleting specially. Suppose `hello` and
 `world` got same hash result. And then `hello` goes to bucket 10 while `world` goes to bucket
 11. If you delete `hello`, then bucket 10 is empty. Now if you get or set `world`, the hash
  result is same as `hello` then it will go to bucket 10 but not bucket 11 since no hash
  conflict happens (`hello` is deleted). So you cannot get previous inserted `world` and you
  may have two `world` as keys if you set it.

To avoid this, you need to mark the deleted bucket so that it won't be treated as empty
 bucket later. Only open address hash table has this problem.

#### hash security
We know hash table has O(1) for get,set,add and delete operations. But it's average case.
 In worst case, it can be O(n). Think about all keys are hashed into same bucket. To avoid
 this, the hash algorithm is very important.

```
// a bad hash function, all key go to bucket 1
    function hash(key) {
        return 1
    }
```

The hash function should generate uniform output to reduce possibility of hash conflict.
And if it receives keys from untrusted source, the hash function should use some rand bytes
 as seed to avoid hash attack (attacker may construct keys that go to same bucket if hash
 result is determinable).

### rehash
The undergound array of hash table always starts from a small size since it doesn't know
 how much data it will fill (a large one will waste memory). It's 8,16 or 32 sometimes.
 Use power of 2 so that the modular calculation is fast. As you fill more and more data,
 it must resize to a large table. Otherwise, too much key value pairs will go to same
 bucket then the performance will decrease a lot.

Rehash is the process of resize. You need to hash each key again that why we called it
 rehash. For example, when the size is 8, both hash value 3 and 11 go to the bucket 3.
 But when you resize it to 16 bucket, 3 goes to bucket 3 while 11 goes to bucket 11.

If you need to load too much data, rehash may happen many times. To avoid this, some
 languages (Java) support to intiate a hash table with a size as hint. Then this hash
 table may start from a size that near the hint size. This can avoid many rehash and save
 cpu and memory a lot.

### load factor
The undergound array will resize if the hash table grows too big or shrinks to a small
 one. But when will it resize? For a hash table with array size 8, will it resize to 16
 when it is half full or totally full?

The `load factor` determines it.

    load factor = capacity / array_size

For open address hash table, each bucket will have only one element so that load factor
 cannot be greater than 1. For python which chooses the open address implementation, the
 load factor is 2/3. So that it will resize when it is 2/3 full.

The `HashMap` of Java is a chained hash table. And the default load factor is 0.75. And
 it is very flexible. You can create it with a intiatial size and load factor.

### map in C++
Not all `Map` are implemented using hash table. The `std::map` of C++ is usually implemented
 using red black tree. So the time complex of get/set/add/delete operations is O(lgN).
 Slower than hash table. But it uses less memory and keys are ordered.

C++ also provides the `std::unordered_map` which is implemented using hash table.


### references
[python source: load factor](https://github.com/python/cpython/blob/master/Objects/dictobject.c#L402)
[java source: HashMap](https://github.com/openjdk/jdk/blob/master/src/java.base/share/classes/java/util/HashMap.java#L249)
[ccp reference: std map](https://en.cppreference.com/w/cpp/container/map)
