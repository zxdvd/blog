```metadata
tags: postgres, proxy, pgbouncer
```

## postgres proxy: pgbouncer
`pgbouncer` is one of the most famous connection pool (proxy) of postgres. `pgbouncer`
 is single thread and it uses `libevent` to do multiplex underground.

It supports 3 pool modes: session, transaction and statement. `statement` mode means
 session can be used by another client just after it finishes a statement. Then of
 course it doesn't allow multi-statements transactions. `session` is the default mode.

### session reset
Default session reset command is `DISCARD ALL` and you can overwrite it using
 `server_reset_query`. `pgbouncer` only reset runs this in session mode by default.
 But you can force it to run in transaction mode by enable `server_reset_query_always`.

### pool size and connection limit
Common settings about pool size and connection limit are:

- default_pool_size
- min_pool_size
- max_client_conn
- max_db_connections
- max_user_connections

### timeouts
Supported timeouts are:

- server_idle_timeout
- server_connect_timeout
- client_login_timeout
- dns_max_ttl

Following are timeouts thought to be dangrous:

- query_timeout: long running query will be cancelled
- client_idle_timeout
- idle_transaction_timeout: this will close clients that were idle in transaction for a long time

### tcp and tls
There are many ordinary tcp and tls settings, you can get detail in official
 documentation.

- so_reuseport: from version 1.12.0, you can set `so_reuseport=1` then multiple pgbouncer
 instances can listen on a same port and kernel will distribute connections to these
 instances. You can enable this to utilize multiple cpu cores.

### authentication
`pgbouncer` handles the authentication by itself so that passed clients get sessions from
 the connection pool. It can do authentication via a text file or using sql query.

You can add a user by append a line into a text file like following:

    "zxd" "md5393e4e684769870f13b94fad70150ba4"

Default auth type is `md5`. If you want to add a user `zxd` with password `12345`, you need
 md5 of string `password+user` (`12345zxd`). You can calculate it via shell

    echo -n '12345zxd' | md5sum

Attention, you need `echo -n` since echo will add `\n` at the end by default and then you'll
 md5 of `12345zxd\n` which is not expected.

### management console
`pgbouncer` has a internal management console that you can get many statistics about
 clients, backends and pools. You need to set `admin_users` and related password, then
 you can connect the console using database `pgbouncer` like other normal database. Issue
 command `show stats` to get result.

### demo config
A very simple demo config:

```
[pgbouncer]
listen_port = 6400
listen_addr = 127.0.0.1
user=zxd
auth_file=./users.txt
admin_users=admin

[databases]
myorder = host=192.168.10.10 port=3433 dbname=order user=order_app password=123456
```

### references
- [pgbouncer doc: config](https://www.pgbouncer.org/config.html)
- [pgbouncer doc: usage](https://www.pgbouncer.org/usage.html)
