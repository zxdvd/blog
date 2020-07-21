```metadata
tags: web, http, cache, WIP
```

## http: cache

### 304 Not Modified cache
The `304 Not Modified` http status is used for conditional request. So what is conditional
 request?

When you `GET` a url for the first time, the server response can attach last modified time
 or hash of the content in the header. Then if you `GET` same url (reload or refresh page),
 the http client (like browser) can send a conditional request that has the modified time
 or hash attached. The server will deal with the request and get the response body. However,
 it will compare the modified time or hash of this new response body with that in request
 header. If the result indicates no change, the server will reponse with status 304 and
 empty body.

From above we can find that the backend server still needs to deal with the request and
 compute the result. It doesn't save the backend but it does save the bandwidth a lot since
 no need to transfer the same response body again and again.

Currently, there are two ways to do the comparing. One is modified time. The server will
 set response header `Last-Modified` with last modified time of the content. It is often
 mtime of the file. Then client can use this time in request header `If-Modified-Since`
 in later requests.

Another way is comparing hash of body. You can hash the body and set value in response
 header `ETag`. Client can use this hash result in request header
 `If-None-Match: "etag_value1", "etag_value2"` in later requests. Currently http related
 protocol doesn't specify the hash algorithm, you can choose any one.

Attention, if both methods are used, the `ETag` has precedence over the modified time.

### references
- [MDN: ETag](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/ETag)
- [MDN: If-None-Match](https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/If-None-Match)
