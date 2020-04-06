<!---
tags: redis, command
-->

## the whole picture about how redis process a command

Take the simple `set foo bar` as a example, I will show how the server deals with it.

I use the vscode and remote ssh extension to debug the redis server process. You can
 also use the gdb cli.

Key configs in vscode's launch.json file.

```
            "type": "cppdbg",
            "request": "attach",
            "program": "/code/redis/src/redis-server",
            "processId": "26152"
```

I set a breakpoint in the `setGenericCommand` function of t_string.c file since the
 command to test is `set`.

Then execute `set foo bar` command at the redis-cli. The server will receive the command
 and stop at the breakpoint.

You'll find the redis server has multiple threads (4 threads when I was testing). I've
 explained this in another post that redis has background workers.

Choose the thread that marked as `paused on breakpoint`. That's the main thread. And
 we can got the frame stacks.

```
setGenericCommand(client * c, int flags, robj * key, robj * val, robj * expire, int unit, robj * ok_reply, robj * abort_reply) (/code/redis/src/t_string.c:71)
setCommand(client * c) (/code/redis/src/t_string.c:146)
call(client * c, int flags) (/code/redis/src/server.c:3199)
processCommand(client * c) (/code/redis/src/server.c:3561)
processCommandAndResetClient(client * c) (/code/redis/src/networking.c:1664)
processInputBuffer(client * c) (/code/redis/src/networking.c:1759)
readQueryFromClient(connection * conn) (/code/redis/src/networking.c:1867)
callHandler(ConnectionCallbackFunc handler, connection * conn) (/code/redis/src/connhelpers.h:76)
connSocketEventHandler(struct aeEventLoop * el, int fd, void * clientData, int mask) (/code/redis/src/connection.c:275)
aeProcessEvents(aeEventLoop * eventLoop, int flags) (/code/redis/src/ae.c:465)
aeMain(aeEventLoop * eventLoop) (/code/redis/src/ae.c:523)
main(int argc, char ** argv) (/code/redis/src/server.c:5063)
```

So this is the main picture of function call path of a `set` command. Most of other
 commands are same to this.

In vscode, you can choose any function to show function stack.

The `main` calls the `aeMain` finally. The `aeMain` is a simple dead loop that polls
 events and process events.

When the server received command from client, the `epoll` or `select` will get readable
 events. And the `aeProcessEvents` will deal with it.

```c
             if (!invert && fe->mask & mask & AE_READABLE) {
                fe->rfileProc(eventLoop,fd,fe->clientData,mask);
                fired++;
            }
```

The `connSocketEventHandler` deals with the network events. It will call read_handler
 or write_handler accordingly.

When a client was created, it set `readQueryFromClient` as read handler:

```c
client *createClient(connection *conn) {
    client *c = zmalloc(sizeof(client));

    if (conn) {
        connNonBlock(conn);
        connEnableTcpNoDelay(conn);
        if (server.tcpkeepalive)
            connKeepAlive(conn,server.tcpkeepalive);

        connSetReadHandler(conn, readQueryFromClient);

        connSetPrivateData(conn, c);
    ...
```

The `readQueryFromClient` then calls the `processInputBuffer`

The `processCommand` will get the command from command table. Then check authentication
 and acl. Simple command and multiple commands transaction is also handled here.

For simple command, it calls the `void call(client *c, int flags)` (server.c). This
 then calls the `c->cmd->proc(c);` to do the real query. This function also handles
 a lot of statistics and propagation.

The `c->cmd` is a function pointer that points the the related command handler. For
 `set`, it is `setCommand`. Then it is called finally to do the key value set.

This is the whole path of a redis query. Since redis can be seen as single thread,
 it's very easy to add breakpoint to dig into the details and get better understand
 of it.
