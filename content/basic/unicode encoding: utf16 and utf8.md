```metadata
tags: basic, string, unicode, utf8, utf16
```

## unicode encoding: utf16 and utf8

I want to write a little about the history at first.

There is only ASCII with when computer is invented. While computers were more pupolar
 among the world, more character sets are needed and intented, especially CJK ( Chinese,
 Japanese, Korean) character sets.

Then came UCS2 that uses a fixed code unit (2 bytes) which can represent 65535 (2^16-1)
 characters. At that time, they think this is pretty enough.

UCS2 does work well for a long time. And many programming languages use it as internal
 character encoding for string, like java and js. And Microsoft Windows use it as internal
 encoding. It's easy and quick since each character uses a fixed 2 bytes.

However, unicode grows so fast since it characters from more and more languages are adopted
 and many graph characters are also adopted. Then 2 bytes are not enough.

The whole unicode characters are divided into 17 planes, from 0x0 0000 to 0x10 FFFF, each
 with 16 bits.

The first plane is from 0x0000 to 0xFFFF which holds all characters of UCS2, which is also
 the most commonly used. This is called as Basic Multilingual Plane (BMP). So how to encode
 the non-BMP characters?

Then UCS4 (UTF32) comes. Yes, it uses 4 bytes (32 bits) for each characters. Of course this
 is enough now and a long time in the future. But the problem is that it wastes so much
 spaces for latin1 characters and even for CJK characters.

UTF16 extends the UCS2 that it holds all characters of UCS2. And it uses a NOT USED range
 of UCS2 to encoding the non-BMP characters.

The not used range is 0xD800-0xDBFF. Currently, The whole unicode is 21 bits width. UTF16
 uses range 0xDB00-0xDFFF to representing the higher 11 bits and range 0xDC00-0xDFFF for
 the lower 10 bits. So these two code units composes a surrogate pairs which can represent
 whole unicode.

Then UTF16 becomes to a vary-length encoding. It's total compatible with UCS2
 and also supports all unicodes. It works well. The only bad side is that you cannot get
 `str[i]` easily as UCS2 since character length varies.

UTF16 saves a lot of space but the size is still twice of ASCII. It's not so friendly to
 users that use ASCII only.

Then another vary-length character set comes, thus UTF8. It totally compatible with ASCII.
So it only uses 1 byte for ASCII. It uses 2 bytes for unicode in the range 0x0080-0x07FF
 and 3 bytes of others in BMP, and 4 bytes for non-BMP.

UTF8 becomes pupolar and nowadays it is used in more than 95% of web pages (it really saves
 a lot since most html,js,css are ASCII).

### UTF16 and unicode conversion
For unicode code point less than 0xFFFF, just uses a same code unit. For code point larger
 than 0xFFFF, following is the the conversion formula:

```
Supppose C is the non-BMP, <H, L> is the UTF16 surrogate pair

// unicode code point to UTF16
H = Math.floor((C - 0x10000) / 0x400) + 0xD800
L = (C - 0x10000) % 0x400 + 0xDC00

// UTF16 to unicode code point
C = (H - 0xD800) * 0x400 + L - 0xDC00 + 0x10000
```

### UTF8 and unicode conversion
The code unit of UTF8 is 1 byte and a character can be 1 to 4 bytes. Following is the conversion
 table (from wikipedia UTF8).

| Number of bytes | First code point | Last code point | Byte 1   | Byte 2   | Byte 3   | Byte 4   |
|-----------------|------------------|-----------------|----------|----------|----------|----------|
| 1               | U+0000          | U+007F         | 0xxxxxxx |          |
| 2               | U+0080          | U+07FF         | 110xxxxx | 10xxxxxx |          |
| 3               | U+0800          | U+FFFF         | 1110xxxx | 10xxxxxx | 10xxxxxx |          |
| 4               | U+10000         | U+10FFFF       | 11110xxx | 10xxxxxx | 10xxxxxx | 10xxxxxx |

Take `我` as a example, the code point is 0x6211. It's in the range U0800-UFFFF.  UTF8
 needs 3 bytes for it. It is `0110 0010  0001 0001`. Let's divide it to 3 parts:
 `0110 001000 010001` and fill the 3 parts to 3 bytes. Then we get `1110 0110`, `10 001000`
 and `10 010001`, thus `\xe6\x88\x91`.

### byte order (little endian, big endian)
For encodings with code unit more than 1 byte, like UTF16 and UTF32, the byte order is
 influenced by computer's endianness. For example:

```python
>>> '🙂'.encode('utf_16_le').hex()       // little endian
'3dd842de'
>>> '🙂'.encode('utf_16_be').hex()       // big endian
'd83dde42'
>>> '🙂'.encode('utf16').hex()           // default is little endian in python3, what's fffe???
'fffe3dd842de'
```

### BOM (byte order mark)
For UTF16 and UTF32, you may get problems when communcating among machines that mixed
 with little endian and big endian. BOM is used to deal with this.

BOM is a fixed bytes at the beginning of file or stream that is used to hint the endian.
For example, `FFFE` is used for little endian while `FEFF` is for big endian.

From python example of above section, we can find that `str.encode('utf16')` add `fffe`
 at the beginning. So we know it is little endian by default.

### references
- [wikipedia: UCS](https://en.wikipedia.org/wiki/Universal_Coded_Character_Set)
- [wikipedia: UTF8](https://en.wikipedia.org/wiki/UTF-8)
- [blog: javascript's internal character encoding](https://mathiasbynens.be/notes/javascript-encoding)
