```metadata
tags: js, node, basic, string, unicode
```

## js basic: string, bytes, unicode conversion

### char, integer and hex
int to hex

    Number(127).toString(16)     // got string '7f'
    a = 127, a.toString(16)      // OK
    127.toString(16)             //  WRONG, you cannot raw int directly, you need to box it first

hex to int

    parseInt('7f', 16)


### string and byte array

    const buf = Buffer.from('abc 我们')           // default encoding is utf8
    const buf = Buffer.from('abc 我们', 'utf8')   // same as above

    const s1 = buf.toString()                // get origin string
    const s1 = buf.toString('utf8')          // we can specify the encoding, utf8 is default

### byte array and hex

    const buf = Buffer.from('abc 我们')           // get a byte array
    const hex = buf.toString('hex')       // get string '616263e68891e4bbac'

    const buf1 = Buffer.from(hex, 'hex')  // get buffer from hex string

### binary and byte array

    const binary1 = '\x61\x62\x63\xe6\x88\x91'    // a binary string, length is 6
    const buf1 = Buffer.from(binary1, 'binary')   // binary string to buffer
    buf1.toString() === 'abc我'

    buf1.toString('binary')                       // buffer to binary string

### string and code points

Attention first:

    The `str.length` is length of unicode code units, equals to `byte-length/2`.
    It's not unicode character length. For example:
        console.log('🙂'.length)         // got 2

    In javascript, `s[i]` will the get i-th unicode code unit but not i-th character. While it's the i-th character in python.

string to code points

    const s1 = '123𠮷我们abc𠄎@@'
    const codePoints = []
    for (let i = 0; i < s1.length; i++) {
        const utf16Char = s1.codePointAt(i)
        codePoints.push(utf16Char)
        // for unicode large than 2 bytes(0xFFFF), utf16 use 2 code units to represent it
        // codePointAt awares of this and return code point at first index so we can skip the next
        if (utf16Char > 0xFFFF) {
            i++
        }
    }

code points to string

    // code points for '123𠮷我们abc𠄎@@', got from above code
    const codePoints = [ 49, 50, 51, 134071, 25105, 20204, 97, 98, 99, 131342, 64, 64 ]
    const s1 = String.fromCodePoint(...codePoints)

### charCodeAt vs codePointAt
String has both `charCodeAt` and `codePointAt` methods, so what's the difference?

You may find that for almost every character, they got same result.

    const s1 = 'abc我们是123!@#'
    for (let i = 0; i < s1.length; i++) {
        assert(s1.charCodeAt(i) === s1.codePointAt(i))
    }

Actually, `charCodeAt` works on UTF16 code units, thus 2 bytes (0x0000-0xFFFF). While
 `codePointAt` is well aware of unicode. It will do some transformations.

We know that even though most frequent used characters are in range 0x0000-0xFFFF. But
 there are still many rarely used characters, like emojis that beyond this range. Then
 UTF16 have to use two code units (4 bytes) to represent them.

`CodePointAt` is unicode awared. It can convert characters that use two code units to
 a single unicode code point.

    '🙂'.codePointAt(0).toString(16)   // got '1f642' which is code point for the emoji
    '🙂'.codePointAt(1).toString(16)   // got 'de42', the second code unit of the emoji, can be ignored

We can see the polyfill of `codePointAt` in Ref 1. Actually it uses `charCodeAt` underground.


### references
1. [MDN: codePointAt polyfill](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String/codePointAt#Polyfill)
2. [blog: javascript's internal character encoding](https://mathiasbynens.be/notes/javascript-encoding)
