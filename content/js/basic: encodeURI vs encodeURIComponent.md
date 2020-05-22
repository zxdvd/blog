<!---
tags: js, encoding
-->

## js: encodeURI vs encodeURIComponent

encodeURI won't encode following chars while encodeURIComponent will.

    ;,/?:@&=+$#

A simple example:

```js
> a = "http://u:pass@www.abc.com/123abc?name=张三&age=18#login"
'http://u:pass@www.abc.com/123abc?name=张三&age=18#login'
> encodeURI(a)
'http://u:pass@www.abc.com/123abc?name=%E5%BC%A0%E4%B8%89&age=18#login'
> encodeURIComponent(a)
'http%3A%2F%2Fu%3Apass%40www.abc.com%2F123abc%3Fname%3D%E5%BC%A0%E4%B8%89%26age%3D18%23login'
```

The `:/&#@` that commonly used in url are also encoded when using `encodeURIComponent`.

If you need to use it in url query string like `abc.com/login?redirect=REDIRECT`, the `REDIRECT`
 here must be encoded using `encodeURIComponent`. Otherwise, if it has substr like `&a=b`, it
 will be parsed as query string of the new url but not the `redirect` params.

```js
    location.href = `abc.com/login?redirect=${location.href}`        // bad
    location.href = `abc.com/login?redirect=${encodeURIComponent(location.href)}`     // good
```

### references
- [devdocs: js encodeURI](https://devdocs.io/javascript/global_objects/encodeuri)
