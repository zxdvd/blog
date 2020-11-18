```metadata
tags: postgres, proxy, pgbouncer
```

## postgres: proxy overview

You may need a proxy behind of your postgres servers due to many reasons like following:

- hide endpoints of postgres servers, using unified endpoint

- read write splitting to improve performance

- keep a connection pool and reuse it to avoid too many idle processes in backend server

We know there are L4 proxy and L7 proxy for http servers. L4 proxy is simple RAW
 tcp proxy while L7 proxy understands the application protocol and then has more
 features based on parsed information.

Postgres has similar L4 proxy and L7 proxy. I think any L4 proxy like haproxy or nginx also
 suites for postgres since it just proxies TCP stream. But its function is limited. It
 doesn't support load balancing or connection pool.

L7 proxy like [pgbouncer](https://www.pgbouncer.org/usage.html) and
 [odyssey](https://github.com/yandex/odyssey) has more features.

Workflow of a L7 proxy looks like following:

```
        client_auth               backend_auth
user  ----------------> proxy pool -------------------> backend1 - db1
                            ∟-------------------------> backend2 - db2
                            ∟-------------------------> backend3 - db3
```

### connection pool
We know that postgres will start and keep a backend process for each connection. Without
 connection pool, applications either use a single connection for all queries or each query
 uses a fresh new connection. A single connection leads to bad performance while each query
 a new connection may lead to too many connections and postgres backend processes start/die
 frequently.

Then you need a connection pool. You can restrict pool size to avoid too many backend
 connections and applications can query concurrently and safely.

Some ORMs or database drivers have built-in connection pool. An external connection pool
 is still neccesary in some cases. Suppose you have a node.js application that has a pool
 size of 20 and you scale it with 200 containers. Then you may have 4000 postgres backend
 processes though many are idle. With one or more external pools, you only have 4000
 connections to the external pools with much fewer shared postgres backends.

#### connection pool mode
The connection pool may support many mode, the most common are `session` and `transaction`.
 The `session` pool mode means that a client always hold the same postgres backend until
 it disconnected. Other clients cannot use it even it is idle. The backend session is
 returned to the pool when the client disconnected and then it can be used by another
 client. And often a `DISCARD ALL` is issued to reset the session.

The `transaction` pool mode means that the client only hold the session till end of the
 transaction. Another client can use the same session when the transaction ended. By
 this way backend sessions are more busy and you may need few backend sessions. But
 session settings are shared between different clients and it may lead to problems.
 Of course, you can issue `DISCARD ALL` or other commands to reset the session, but
 reset too often may affect performance a lot.

I think `session` mode is always suggested. You can try `transaction` mode to achieve
 better performance if you don't have client specific session settings.

### read/write splitting
For very large scale applications that reading is far more than writing, read/write
 splitting can improve performance dramatically, especially for cases that don't need
 strong consistence.

You can achieve read/write splitting at application level. Using two database endpoints,
 one for writing and one for reading. You decides whether the query uses the read-only
 endpoint or not. You need to modify your codes a lot and you may get trouble if some
 ORMs don't support it well.

With L4 proxy, it is possible to achieve transparent read/write splitting. It means that
 you don't need to change you code or only change very few codes. The query knows that
 whether it's a simple select query or not and whether it's in a read-only transaction
 or not. It can route reading to replication server safely. It's a single unified endpoint
 to your applications.

Since it's transparent and needs no code change or very few changes, it's very suitable
 for old projects that needs to improve performance.

### high availability
For a high availability postgres cluster, you need a proxy in front to do health check
 and failover.

### customized authentication
Many proxies support authentication using customized sql query. You can define function
 to achieve flexible authentication. For example, you can manage users of many applications
 to many postgres servers using a single table.
