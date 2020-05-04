<!---
tags: web, http, cookie
-->

### http cookie: same-site attribute
SameSite is a new attribute of cookie started by google chrome and followed by other
 popular browsers. The aim is providing another way to prevent CSRF.

The key of CSRF is that the attacker can send request to A domain from B domain with
 cookies. So that it may do things from add/remove posts to transfer money. The `SameSite`
 attribute provides a way to point that whether the browser can take the cookie with
 the request together or not.

### SameSite options
Currently, SameSite has three options: Strict, Lax, None.

Strict is the most strict one.  The cookie will only be sent in a first-party context. It
 will not be sent with request initiated by third party sites. It means that if you click
 a `<a>` tag to open a new tab of another site, the first GET request won't have cookies.

Lax is not so strict. It needs the first-party context. However, it will send cookies on
 `<a>` tag for new site while `Strict` won't. Lax will be the default behavior if no
 SameSite attribute is set (from chome 80).

None is like the old behavior that cookies will be sent in all contexts. In browsers that
 support SameSite, `None` requires the `Secure` attribute.

### how to determine SameSite?
So you way wonder that what is a SameSite request? Of course, same domain is SameSite.
 But are `a.b.com` and `b.com` same site? How about `a.b.com` and `c.b.com`?

For most domain like `A.B`, we treat `A.B` and `a.A.B` and `a.b.A.B` as same site. However,
 there are some exceptions. Think about `github.io`, it hosts static blogs for each user.
Of course `zxdvd.github.io` and `other.github.io` are not same site. This case is very
 common for blog hosts and CDN hosts and domains like `com.cn`.

It seems that there isn't logical rules about same site. So how to determine it? There is
 a list about these public suffixes. Popular browsers will use it and you can submit to
 update this list. Site about the list: [public suffix list](https://publicsuffix.org/list).

### how to set it
Most time, the default `Lax` is enough for you, so you needn't to do anything. But if you
 want to set it to other options, one more thing need to take care about: what will happen
 on old browsers that do not recognize this `SameSite` attribute?

And if you want to set to `None`, remember that it needs the `Secure` attribute.

### references
- [web.dev: samesite cookies explained](https://web.dev/samesite-cookies-explained/)
