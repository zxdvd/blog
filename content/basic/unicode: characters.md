```metadata
tags: basic, unicode
```

## unicode: characters

I've got several problems that associate with some special unicode characters, control
 characters especially.

Users complained that they can not search something they had generated. And then I found
 that the search keyword has non printable characters while the generated doesn't have.

    We always limit the length of user input, but I think we should also limit the character
    range to avoid these weird problems.

The unicode not only includes all ascii control characters, but also extends more
 complexed characters, like bidirectional format characters.

### unicode categories
The unicode is divided into following main categories:

- Letter
- Mark
- Number
- Punctuation
- Symbol (math, currency, modifier, other)
- Separator (space, line, paragraph)
- Other (control, format, surrogate, private use, not assigned)

Most problems came about control and format chars since they are invisible.

### unicode control characters
C0 (\u0000-\u001F, \u007F) comes from ASCII. C1 (\u0080-\u009F) comes from ISO 8859.

### bidirectinal text control
Text displays from left to right in most languages but not all. Unicode adds some control
 characters to support format as right to left. Currently, there have 12 chars: (\u061C,
 \u200E-\u200F, \u202A-\u202D, \u202E, \u2066-\u2069).

### printable characters
It's hard to enumerate all control characters. Therefore, it is suggested to pick out
 all printable characters from user input instead of filter out control characters.

It's easy to enumerate common used characters and we can make it strict and turn loose
 gradually.

Following are common used printable characters:

const printableChars = /[\u0020-\u007e\u00a0-\u00ff\u4e00-\u9fff\u3400-\u4dbf]/g
- \u0020-\u007E          ASCII (a-zA-Z0-9!"#$% and so on)
- \u00A0-\u00FF          Latin-1 supplement (like ¡¢£§©®¼ÀÃÈÜè)
- \u4E00-\u9FFF          Common Chinese chars (has 20992 chars), this is enough most time
- \u3400-\u4DBF          Rarely used Extension A (6592 chars), like 㐀㐁㐂㐃㐐㐑㐒㐓
- \u2000-\u2A6DF         Rarely used Extension B (165600 chars), most of them cannot be displayed correctly on macOS Catalina

### regex to filter out printable characters
I'd like to use regex to filter out printable characters, like following:

```js
const printableChars = /[\u0020-\u007e\u00a0-\u00ff\u4e00-\u9fff\u3400-\u4dbf]/g  // LATIN and Chinese

function normalStr(s) {
  return `${s}`.match(printableChars).join('')
}
```

### space characters
It's common to use space as separator. However, unicode has many different spaces. `\u0020`
 is the most common one. `\u00A0` (no-break space) appears in html a lot as `&nbsp;`. Ref. 4
 is the list of all whitespaces.

It depends on you to decide whether considers other whitespaces or not, but we'd better know
 them.


### references
1. [wikipedia: list of unicode characters](https://en.wikipedia.org/wiki/List_of_Unicode_characters)
2. [wikipedia: unicode control characters](https://en.wikipedia.org/wiki/Unicode_control_characters)
3. [stackoverflow: Chinese character range](https://stackoverflow.com/questions/1366068/whats-the-complete-range-for-chinese-characters-in-unicode)
4. [unicode: space category](https://www.fileformat.info/info/unicode/category/Zs/list.htm)
