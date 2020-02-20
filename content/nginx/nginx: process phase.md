<!---
tags: nginx, internal
-->

Nginx divided the processing of a request to multiple phases. It will register one or
 many handlers at each phase. Following are the phases copied from the
 [develop guide](http://nginx.org/en/docs/dev/development_guide.html#http_phases).
And I changed a little.

- NGX_HTTP_POST_READ_PHASE — First phase
    - ngx_http_realip_module
- NGX_HTTP_SERVER_REWRITE_PHASE
    - rewrite directives defined in a server block (but outside a location block) are processed.
    - The ngx_http_rewrite_module installs its handler at this phase.

- NGX_HTTP_FIND_CONFIG_PHASE
    - a location is chosen based on the request URI.
    - No additional handlers can be registered at this phase.
- NGX_HTTP_REWRITE_PHASE
    - Same as NGX_HTTP_SERVER_REWRITE_PHASE, but for rewrite rules defined in the location.
- NGX_HTTP_POST_REWRITE_PHASE
    - Special phase where the request is redirected to a new location if its URI changed during a rewrite.
    - This is implemented by the request going through the NGX_HTTP_FIND_CONFIG_PHASE again.
- NGX_HTTP_PREACCESS_PHASE
    - ngx_http_limit_conn_module and ngx_http_limit_req_module
- NGX_HTTP_ACCESS_PHASE
    - verified that the client is authorized to make the request.
    - ngx_http_access_module and ngx_http_auth_basic_module
- NGX_HTTP_POST_ACCESS_PHASE
    - Special phase where the satisfy any directive is processed.
- NGX_HTTP_PRECONTENT_PHASE
    - ngx_http_try_files_module and ngx_http_mirror_module
- NGX_HTTP_CONTENT_PHASE
    - Phase where the response is normally generated.
    - ngx_http_index_module or ngx_http_static_module.
    - They are called sequentially until one of them
    produces the output.
    - It's also possible to set content handlers on a per-location basis. If the
    ngx_http_core_module's location configuration has handler set, it is called as the content handler
    and the handlers installed at this phase are ignored.
- NGX_HTTP_LOG_PHASE
    - ngx_http_log_module

A request will go through these phases one by one and it may go back and forward sometimes.
You need to know the phase that a module worked at to analyze and debug problems.

Recently I got a problem when testing the `limit req` feature. I setup a host like following

```
limit_req_zone $binary_remote_addr zone=one:10m rate=1r/s;
server {
        listen 80;
        server_name test.local;

        location / {
                limit_req zone=one burst=5;
                return 200 'hello world!';
        }
}
```

Then I use `wrk` to test it. However, it seems that the `limit req` doesn't work. And I didn't
 get any error. I got a qps of 29k and without any 503 error.

Then I change the config again and again and didn't find any useful information. I used to think 
maybe there is bug on nginx of macos and I tried on linux and got same result.

Finally, I commented the `return 200 'hello world!';` and replaced with a normal 
`root /tmp; autoindex on;`, `limit req` worked as expected.

So what's wrong with the `return` directive?

`return` is a directive of the `ngx_http_rewrite_module` module.The rewrite module works
 at the phases before the `limit req` which works at the `PREACCESS` phase.

### references
- [nginx develop guide: http phases](http://nginx.org/en/docs/dev/development_guide.html#http_phases)
