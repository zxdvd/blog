```metadata
tags: nginx, rate-limit
```

## nginx request rate limit

### why limit rate?
- prevent request flood, malicious attacks
- protect backend applications and datbases
- make request flow more stable

### a simple demo

```
limit_req_zone reqzone1 zone=test_local:1m rate=10r/s;

server {
        listen 80;
        server_name test.local;

        location / {
                limit_req zone=test_local;
                root /var/www/test;
        }
}
```

You need to define a `limit req` zone at the `http` context (cannot define at
 server block). The **reqzone1** at here is key of the zone. A constant string
 at here means that all requests of this zone share the same rate. You may see
 config like following:

    limit_req_zone $binary_remote_addr zone=one:10m rate=1r/s;

Here variable **$binary_remote_addr** is the key, then the rate is applied to each
 distinct client ip address. So it limits request rate from each ip address.

You can also use other variables like `$server_name`, `$request_uri`, `$http_cookie`,
 it depends on what's kind of rate limit you want, by ip, by user id, or url or
 combined.

Then `test_local` and `one` in above examples are zone names while the `1m` and
 `10m` are memory size limit of the zone. Suppose that you use `url` and `ip` as key,
 it may use too much memory then you can set a limit of the zone.

And the `rate=1r/s` in the end means at most one request every second. You can also
 set it like `rate=30r/m` which means 30 requests per minute.

After defined the zone, then we need to choose where to limit. We can use the `limit_req`
 directive at http context, server context or location level. At the above example,
 we apply it to `location /`.

You can use same zone at multiple location and they will share same rate limit. If
 you want each location has separate rate limit, you need to define separate zones.

### burst and delay
If you set a limit rate as 10r/s, nginx will accept and process one every 100ms. If
 another request comes in within 100ms, it will be rejected with a 503 error by default.
However, most web applications will have burst requests at initial stage (css, js, fonts)
 and very few at later stages.
 A strictly one request every 100ms otherwise error will block many users. You can use
 `limit_req zone=one burst=10;` then if 10 requests come in, you won't get 9 errors.
One will be processed immediately, 9 will be put in the waiting list and processed every
 100ms. If there are more requests and the waiting list is full, there will be rejected.
The `burst` really helps a lot, but it means that you need to wait 1s to get content in
 browser if it has 10 initial requests. You can add set it like following

    limit_req zone=one burst=10 nodelay;

The `nodelay` means that 10 requests will be processed at the same time. It just limits
 no more than 10 requests in 1 second not but one request every 100ms.

There are other directive that can control the http code and log when request is rejected.

### processing phases
Attention, following configuration won't take effect when testing `limit req`

```
    location / {
            limit_req zone=test_local;
            return 200 'hello world!';
    }
```

The `ngx_http_limit_req_module` works at the `NGX_HTTP_PREACCESS_PHASE` which
 is behind the rewrite phase that the `return` directive belongs to.

