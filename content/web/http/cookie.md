<!---
tags: web, http, cookie
-->

## http cookie
We know that the HTTP protocol is stateless. But browser can use cookies to store state
 of a session.

### character and encoding in cookie
HTTP protocol has some restrictions about characters since it is text protocol and it uses
 special characters as delimiters.

The `Set-Cookie` header is like following:

```
Set-Cookie: <cookie-name>=<cookie-value>; Domain=<domain-value>; Path=<path-value>
```

Then we can see that characters like `=`, `;` and `\r\n` may get ambiguities. You can
 check the RFC or referenced stackoverflow post at bottom to get detailed character
 limits. However, I suggest to use only alphanums and `-_.`. If you want to use other
 characters like CJK, you can do a url encoding.

### cookie domain
The cookie domain may have a leading dot and may not. Without a leading dot, like
 `example.com`, means that it should match `example.com` exactly. It cannot be used
 by any subdomain.

With a leading dot like `.example.com`, means that it will be used by both `example.com`
 and all subdomains.

For frontend, set a cookie without domain, like `document.cookie = 'hello=world'`, the
 domain will be the location host without leading dot. And if you set cookie with domain,
 like `document.cookie = 'a=b;domain=example.com'`, browser will add leading dot no matter
 you specified it or not.

### get cookie info
Though cookie has many properties. Application can only get plain key value pairs.
The cookie in request header is something like `k1=hello; k2=world; k3=abc`. The browser
 will select available cookies to concat to the `Cookie` header. Then the server side
 can only parse it to an array of kv pairs. There is no information about domain, path,
 expires.

For the frontend, it can access cookies via `document.cookie`, but it is also simple
 plain kv pairs like above. No other information.

But for developer, you can get detailed information about cookies using develop tools.

### remove cookie
You can only remove visible cookies. You can not remove cookies from subdomain or other
 domain. There is no remove or delete api for cookies. But you can achieve it via set
 `max-age` to 0 or `expires` to an outdated date.

Attention: all of name, domain and path should match at same time to determine the cookie
 to be removed.

```
document.cookie = 'test=hello;domain=.example.com;path=/;max-age=0'
```

### same cookie name
We can set cookie that have same names but different domain or path. We treat them as
 same names. But actually, name, domain and path together make the unique cookie
 identifier.

You may have two cookies with same name and different domains like `example.com` and
 `a.example.com`, then both of them are visible at host `a.example.com`. If you think
 that the one with domain `a.example.com` has higher priority, that's wrong. Application
 will get text like `k1=hello; k1=world`. Yes, same key, two values. You don't know
 which one is subdomain's.

So be careful about the cross subdomain's cookie design, you need to know which is shared
 and which is not. You'd better not use same names for different subdomains.

### multiple sites cookies in same host
A site (a browser tab) can have requests from many different origins. So for a tab, it
 may have cookies from different domains. The browser will only send the cookies that
 matched with host for each request.

### references
- [cookie allowed chars](https://stackoverflow.com/questions/1969232/what-are-allowed-characters-in-cookies)
