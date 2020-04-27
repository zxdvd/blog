<!---
tags: go, basic, bytes, string, runes
-->

## go basic: bytes, string and runes
You may find bytes, string and runes about characters. So whatare the differences between
 them and which one to use?

### bytes
Byte is actually alias for `uint8`, defined as `type byte = uint8`. And bytes or byte slice
 are just slice of byte like `s1 := []byte("hello world")`. Each element of the slice is
 a single byte (8 bits). We often use it with binary data, raw data or ascii characters.

### string
The string is like a read only slice of bytes. This is different from string of python.
Following are attentions you need to take care about string:

- string is just RO slice, then len(str) is actually length of the slice

```go
    s1 := "你好，世界!"
    fmt.Printf("length: %d\n", len(s1))        // get 16 but not 6
```

- string is just RO slice, it's no need to be VALID string, you can store any bytes

- range loop string will decode string as utf8 and return unicode code points (rune)

```go
    s1 := "你好!!"
    for i, ch := range s1 {
        fmt.Printf("index: %d, char: %s, code: %U\n", i, string(ch), ch)
    }
// output
    index: 0, char: 你, code: U+4F60
    index: 3, char: 好, code: U+597D
    index: 6, char: !, code: U+0021
    index: 7, char: !, code: U+0021
```

    If you don't want this, you can loop string use `for i := 0; i < len(s1); i++ {}`
     or `for i, b := range []byte(s1) {}`.

### runes
Rune is alias of `int32`, defined as `type rune = int32`. Often used to store unicode
 code point. So why rune?

Rune uses fixed width for each character, no matter it's ascii like `a`, or chinese
 like `你`. It will use more memory than string (utf8 encoded) but the benefit is that
 it is O(1) to find character via index.

With utf8 encoded string, it needs to decode from beginning to find the Nth character (
not Nth byte) since each character may be 1 byte to 4 bytes. You cannot jump to the
 Nth character with decoding. However, with runes, just simple jump to the 4 * (N-1) bytes.

### conversion
Conversion between bytes, string and runes.

```go
    s1 := "你好!!"
    s2 := []rune(s1)     // convert string to []rune
    s3 := string(s2)     // convert rune to string
```
