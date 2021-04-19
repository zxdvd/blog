```metadata
tags: web, browser, CORS
```

## http: CORS

CORS is only a browser feature that let the backend applications knew where the request
 came from and whether to allow or deny the request.

The fetch api has 3 modes: same-origin, no-cors, cors. The same-origin is the default
 one that used for same origin request. If you want to do cors request, you may need the
 other two modes. So what's the difference?

### no-cors mode
For this mode, no need to send the CORS preflight request, but then there is also some
 limitations:

- request method must be CORS-safelisted methods (GET, HEAD, POST). Request won't send
 and no error in console if you use other methods like `DELETE`

- the response type is `opaque`. The js code can only get status code of the response.
 It cannot access headers or body of response though you can see them in network of
 browser's devtools.

So if you only use `GET` or `POST`, and you don't care about the response body, you can
 use this mode.

With this mode, you can send or receive cookies as normal.


### cors mode
If you want to request from another origin, and you don't want to use the `no-cors` (
 uses other method or needs the response body), then you need to use mode `cors`.

There are two kinds of cors request. One is very simple that has no preflight request.
Another needs a preflight request.

#### So what is cors preflight request?
Preflight request is a request to same url with method as `OPTIONS` to ask permission
 for the cors request. Server may allow or reject it accordingly.

The preflight request won't include the credentials. So do not design complex logic for
 this.

#### What is simple cors request?
Actually, there is no official name for it. A simple cors request must match all of following:

- the request method is one of GET, HEAD, POST
- you didn't set special request headers
- content-type header must be one of "application/x-www-form-urlencoded", "multipart/form-data", or "text/plain"

The benefit of simple cors request is that browser won't send the preflight request. Browser
 still needs to check cors related headers of the response.

If it is not a simple cors request, browser will send an `OPTIONS` request of same url first.
If the server responsed properly, browser will then send the origin request.

#### server response for cors request

Server should set some specific headers in the response for cors request so that browser
 know whether to block the request or not. Following are related headers:

- Access-Control-Allow-Origin (must set)

    You must set this if the cors request is allowed. You can set it to `*` which means that
     it is allowed cors request from any origin. Or you can set to a specific `https://xxx.com`
     so that only those from this same origin are allowed.

    Attention, `https://*.xxx.com` is not supported and the origin must be exactly matched.
    Both `xxx.com` and `http://xxx.com/` are invalid since the first one doesn't have `http://`
     while the second one has a path `/`.

- Access-Control-Allow-Credentials

    This controls whether the cors request can include credentials (cookies) or not. Set to
    `true` to allow credentials.

    Attention:

    - it is `true`. You cannot use `TRUE` or `True` or other strings
    - if you set it `true`, the `Access-Control-Allow-Origin` must not be `*`, should be exactly string

- Access-Control-Allow-Methods

    This set allowed http methods. You must use upper case method like `PUT, DELETE`, should
    not use `Put, delete`. It is ok to ignore `GET, HEAD, POST`.

- Access-Control-Allow-Headers

    Some as methods, headers added in cors request must be added at here also. If the `Content-Type`
    is cors safelisted, you can ignore it. Otherwise, you should add it.

- Access-Control-Max-Age

    You can use this header to control how many seconds you want the preflight request
    to be cached so that browser won't send preflight request again and again. Browser
    has max limit of it (10 minutes for chrome, 1 day for firefox).

- Access-Control-Expose-Headers

    By default, only very few headers are exposed to js code, if you want to expose
    other headers. You can add them to this header. Following are automatically exposed:

    - Content-Type
    - Cache-Control
    - Content-Language
    - Expires
    - Last-Modified
    - Pragma

Some articles pointed that `Access-Control-Allow-Methods` and `Access-Control-Allow-Headers` also
 support the `*`. However, they are not implemented by browser currently (tested with chrome 85).
And even it is implemented in the future, it cannot be used when `Access-Control-Allow-Credentials`
 is true.

### cors fetch errors

If the cors request is denied, browser will complain in the console. And if you catch
 the error, you'll get `TypeError: failed to fetch`.


### iframe and cors

When you embedded b.com in a.com, the `Origin` of requests from the embedded b.com is
 b.com, not the outside a.com. So don't worry about that a normal request will become
 cors request when embedded in iframe. This won't happen.

### redirect and cors
When you redirect to different domain using 301 or 302 redirect, the target server should
 deal with cors properly. Otherwise, the redirect will fail due to cors error.

### references
- [fetch spec](https://fetch.spec.whatwg.org/)
- [MDN: CORS](https://developer.mozilla.org/en-US/docs/Web/HTTP/CORS)
