```metadata
tags: redis, sourecode
```

The `util` module cotains some helper functions about string, string match, conversion,
 path helpers.

```c
// glob pattern match, supports *,?,[abc], [^abc], [a-z]
int stringmatchlen(const char *p, int plen, const char *s, int slen, int nocase);
int stringmatch(const char *p, const char *s, int nocase);

// convert strings like `10Gb`, `-128kb`, `30k` to int
long long memtoll(const char *p, int *err);

// counts digits in uint64
uint32_t digits10(uint64_t v);
uint32_t sdigits10(int64_t v);

// string and int, double conversions
int ll2string(char *s, size_t len, long long value);
int string2ll(const char *s, size_t slen, long long *value);
int string2ull(const char *s, unsigned long long *value);
int string2l(const char *s, size_t slen, long *value);
int string2ld(const char *s, size_t slen, long double *dp);
int string2d(const char *s, size_t slen, double *dp);
int d2string(char *buf, size_t len, double value);
int ld2string(char *buf, size_t len, long double value, ld2string_mode mode);

// path related
sds getAbsolutePath(char *filename);
unsigned long getTimeZone(void);
int pathIsBaseName(char *path);
```

### references:
1. [github: redis anet](https://github.com/antirez/redis/blob/unstable/src/util.c)
