```metadata
tags: postgres, proxy, odyssey
```

## postgres proxy: odyssey

`odyssey` is a new project that was open-sourced by yandex. It is said to be used in
 large production setups.

One main feature of `odyssey` is multi-threaded processing. You can config multiple
 workers to serve clients.

### odyssey vs pgbouncer
I think they share many similar features. Both of them work well if you need a simple
 connection pool.

Some differences:

- odyssey can have many workers to handle client connections just like nginx while
 pgbouncer is single threaded like haproxy. But you can setup multiple pgbouncer
 listening on same port to achieve same effect.

- odyssey supports only `linux` since it uses
 [machinarium](https://github.com/yandex/odyssey/blob/master/third_party/machinarium/README.md)
 which relies on `epoll` to handle network events. While `pgbouncer` benefits from
 `libevent` so that it supports many platforms (linux, bsd, windows).

- you can separate users for different databases easily. But for pgbouncer, users are
 shared among all databases. You can use `auth_query` to achieve similar effects but
 its a little complex.

### demo config
A simple demo config:

```
log_to_stdout          yes
log_format "%p %t %e %l [%i %s] (%c) %m\n"
stats_interval 5

listen {
        host "*"
        port 6401
        backlog 64
        tls "disable"
}

storage "order" {
        type "remote"
        host "172.16.0.190"
        port 9999
}

database "order_app" {
        user default {
                authentication "clear_text"
                password "123456"
                storage "order"
                storage_user "readonly"
                storage_password "readonly_123"
                storage_db "order"
                pool "transaction"
                pool_size 30
                pool_timeout 4000
        }
}
```

### references
- [git repo: config](https://github.com/yandex/odyssey/blob/master/documentation/configuration.md)
- [github issue: odyssey vs pgbouncer](https://github.com/yandex/odyssey/issues/3)
