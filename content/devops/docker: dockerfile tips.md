```metadata
tags: devops, docker, dockerfile
```

## docker: dockerfile tips


### COPY vs ADD
`ADD` has more features than `COPY` but it is suggested to use `COPY` since you seldomly
 use extra features provided by `ADD` and the feature may give you unexpected result.

`ADD` supports adds file from remote URL and extracts a directory from a tarball.

```
FROM debian:10
ADD  pgbouncer-1.14.0.tar.gz /app/pgbouncer
RUN find /app
```

Above Dockerfile will extract the `tar.gz` package and place it under `/app/pgbouncer` so
 that you'll get file like `/app/pgbouncer/pgbouncer-1.14.0/src/xxx`.

If you simply replace all `COPY` with `ADD`, you won't get problem at most time but you
 may extract tarballs unexpectedly.

So `COPY` is suggested and use only `ADD` when you need its special features.

### COPY
Some tips about `COPY`:

- You don't need to `RUN mkdir xxx` for the `COPY` target, you can `COPY . /a/b/c/d/e`
 and docker will create directories for you.

- directory and slash. You do need to add slash for command like `COPY dir1 /app`.
It will copy all content in `dir1` to `/app`, you won't get path like `/app/dir1`.
It is same as `COPY dir1 /app/` and `COPY dir1/ /app/`.

If you want `/app/dir1`, you need `COPY dir1 /app/dir1`.

- slash matters when copy single file like `COPY nginx.conf /etc/nginx/`. You MUST
 add `/` suffix to indicate that target is a directory, otherwise it will treat
 target as a regular file if it is not existed.

- You CAN NOT copy multiple directories to target directory. `COPY dir1 dir2 /app/`
 works but it actually something like `copy dir1/* dir2/* /app/` and you won't get
 path like `/app/dir1/xxx` and `/app/dir2/xxx`. You have to use two `COPY` commands.

        COPY dir1 /app/dir1
        COPY dir2 /app/dir2

And you'll get error for `COPY dir1 dir2 /app` since you need to use `/app/` to specify
 that the target is a directory but not a plain file.

- chown supports. From docker 17.09, you can use `COPY --chown user:group` to change
 owner of added files. It saves an extra `RUN chown`.

### references
- [docker doc: best practice - add or copy](https://docs.docker.com/develop/develop-images/dockerfile_best-practices/#add-or-copy)
