```metadata
tags: database, postgres, config, auth
```

## pg-config: client auth config (pg_hba.conf)
The `pg_hba.conf` controlls client authentication. (HBA stands for host-based authentication)
 It matches a connection according username, connecting method, ip address and target database.
 If a line is matched but authentication fails, then the connection fails, it won't try following
 lines. If no line is matched, then connection is denied.

Lines in this file are like following:

     # TYPE  DATABASE        USER            ADDRESS                 METHOD
     # "local" is for Unix domain socket connections only
     local   all             all                                     trust
     # IPv4 local connections:
     hostssl   db1,db2       user1,+user2     127.0.0.1/32            md5
     # IPv6 local connections:
     host    db1,@app.txt    u2,@admins       ::1/128                 password
     # Allow replication connections from localhost, by a user with the
     # replication privilege.
     local   replication     all                                     trust
     host    replication     all             127.0.0.1/32            trust

The first field indicates connection type. `local` means accessing via unix domain socket
 so that it is only applicable to local machine. And it doesn't have ADDRESS field.
 `host` means accessing using TCP/IP (TLS or plain) while `hostssl` is TCP with SSL only
 and `hostnossl` forces plain TCP/IP without SSL.

The second field specifies which database to connect. `all` matches any database. You can
 specify multiple databases at the same time, like `db1,db2`, attention that there should
 not have space around the comma.

If there are too many databases, you can write them in a text file like `dbs.txt` and using
 `@dbs.txt` to include them.

The third field matches the username. And same as database, you can use commas to connect
 multiple usernames or use `@filename` to include from a text file.

for username, you can use `+user1` to match all members of `user1`. The default `user1`
 without `+` doesn't match any member.

The `auth-method` can be any of trust, reject, scram-sha-256, md5, password and pam.
`trust` means no authentication is needed while `reject` will deny the accessing.
`md5` means `md5` or `scram-sha-256` while `password` is plain password which should
 not beused on untrusted network.

Though you can config rules to deny malicious connections, I strongly suggest that you'd
 better not expose postgres in public network. Postgres will fork a process to deal with
 each connection. It's very weak to DDOS attacking.

Internal picture to authentication:

```
PostmasterMain
    status = ServerLoop()
        BackendStartup(port)
            pid = fork_process()
            BackendRun(port)
                PostgresMain(ac, av, port->database_name, port->user_name)
                    InitPostgres(dbname, InvalidOid, username, InvalidOid, NULL, false)
                        PerformAuthentication(MyProcPort)
                            clientAuthentication(port)
                                hba_getauthmethod(port)
                                    // do auth according to auth type
```

### reload
After changing the `pg_hba.conf`, you need to reload the config so that it will take effects.
 You can reload using simple sql query `select pg_reload_conf()` or `pg_ctl reload`.

### references
- [postgres doc: pg_hba.conf](https://www.postgresql.org/docs/12/auth-pg-hba-conf.html)
