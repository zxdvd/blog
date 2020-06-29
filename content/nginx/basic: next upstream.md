```metadata
tags: nginx, basic, upstream
```

## nginx: fallback to next upstream
We can define an upstream that composed of multiple server like following:

```
upstream test1 {
        server test.dev:4500;
        server test.dev:4501;
        server test.dev:4502 backup;
```

This is a kind of load balance. And you can also config it to do fallback. Thus try proxy
 to next one if one server failed.

By default, it will try another server if failed to connect to the current selected one or
 read response header. But you can config it to try another on more scenarios just like
 following. And you can also limit number of tries if you have too much server and you don't
 want the client request to last too long.

```
        location /t1 {
                proxy_pass http://test1;
                proxy_next_upstream error timeout http_503 http_502;
                proxy_next_upstream_tries 3;
        }
```

### Attentions

It won't try if part of the response has been sent to client. For example, if the header
 or chunked body has been sent, and then the backend got some error, nginx won't try another
 backend since you cannot response a http request more than once.

By default, it won't work with http method `POST` and also `LOCK`, `PATCH` since they're
 been thought as non-idempotent. Idepotent means that it is safe to retry for multiple times.
 `GET` won't change anything normally while `PUT` often used to update a specific. We treats
 them as idempotent. However, for `POST`, it is often used to create a resource. A backend
 may have created the resource successful but got error at other part. If nginx retries the
 `POST`, the resource may be created multiple times. So this is non-idempotent. But if you
 don't care about this, you can enable it for `POST` by adding `non_idempotent` qualifier.

### error_page
The `error_page` may have similar effects sometimes. But actually there are many differences.

```
    error_page 502 503 504 = @fallback;

    location @fallback {
        proxy_pass http://ANOTHER_SERVER;
    }
```

Above config will do fallback if got 502, 503 and 504. Looks similar to `proxy_next_upstream`.
However, `error_page` will change the http method to `GET` except for `GET` and `HEAD`. So
 you cannot get same result for `PUT`, `POST` and `DELETE` as `proxy_next_upstream`.


### references
- [nginx official doc: proxy next upstream](http://nginx.org/en/docs/http/ngx_http_proxy_module.html#proxy_next_upstream)
