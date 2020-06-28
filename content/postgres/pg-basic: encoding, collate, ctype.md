```metadata
tags: postgres, pg-basic, encoding, collate
```

## pg-basic: encoding, collate and ctype
While creating a new database, we can find there are 3 similar parameters: encoding,
 lc_collate, lc_ctype. So what's the difference?

```sql
create database test2 encoding 'UTF8' LC_CTYPE='en_US.utf8' LC_COLLATE 'en_US.utf8';
```

### encoding
The encoding determines that which character set to be used to STORE text content on
 disk.

When we write data to database, the text content in sql is just string. The underground
 storage of string (the bytes) can vary very much according the character set used. For
 example, a simple string `abc`, it is `\x616263` when encoded with `utf8`, each is a
 single byte. It is `\x610062006300` when using `utf16`. So `encoding` is needed to
 serialize text content to disk and deserialize when loading from disk.

Be careful when using language specific encoding since it will fail to store data from
 other languages or emoji. `UTF8` is always preferred.

### collation
Collation is all about sorting, it influences `ORDER BY` and comparison (`<, >`).
We know that `a < z` with locale `en_US`, but there may exist another locale that has
`a > z`. This is just an example but it is true that two characters get conflict comparing
 result in two different locales.

Even, a character in one locale may be multiple characters in another. For example, `ch`
 is treated as a single letter in Spanish, Czech, Slovak and so on.

So you you want a locale specific order or characters, you need to define this properly.
If you don't care about locale order, you can simply use `C` or `POSIX` as collation.
It just compares byte by byte. I think it is a little faster.

Attention, a query cannot utilize index if they have different collations.

### ctype
The LC_CTYPE applies to classification and conversion of characters, and to multibyte and
 wide characters. You can get them from `ctype.h`: isalnum, isalpha, isdigit, tolower,
 toupper and many other functions. Different locale may get different result for these
 functions.

### references
- [postgres official doc: encoding](https://www.postgresql.org/docs/current/multibyte.html)
- [wikipedia: single character CH](https://en.wikipedia.org/wiki/Ch_(digraph))
- [postgres official doc: collation](https://www.postgresql.org/docs/12/collation.html)
- [gnu doc: locale categories](https://www.gnu.org/software/libc/manual/html_node/Locale-Categories.html)
- [ICU official user guide](http://userguide.icu-project.org/i18n)
