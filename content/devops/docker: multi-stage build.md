```metadata
tags: devops, docker, dockerfile
```

## docker: multi-stage build

For some projects, the source codes and dependencies are compiled and built to static
 binaries or bundle files and then you don't need source codes and dependencies while
 they are running. But they do waste spaces.

A simple Dockerfile for a frontend project may like following:

```
FROM node:10
WORKDIR /app
COPY --chown=node:node package.json package-lock.json /app/
RUN npm install
ADD --chown=node:node . /app
RUN npm run build
```

After `npm run build`, source files and node_modules can be removed. But if we add
 another `RUN rm -rf node_modules`, it won't work as we expected. The removed directories
 are still kept in intermediate layers since docker uses layered fs.

You can add whole app directory and then use `RUN npm install && npm run build && ...`
 that concats many commands in a single `RUN`. However, by this way you cannot utilize
 cache of `npm install` and you will download dependencies each time you build even
 though no new package is added.

From docker 17.05, it adds the `multi-stage build` feature that resolves this problem
 perfectly. You can define multiple stages in a single Dockerfile and later stages can
 copy files from earlier stages.

```
FROM node:10 as step0
RUN xxx

FROM golang: 1.10 as step1
COPY --from=step0 /app/step0 /app/step0
RUN xxx

FROM nginx:latest
COPY --from=step1 /app/step1 /app/step1
RUN xxx
```

By this way, you can build a clean image with fewer layers.

### references
- [docker doc: multistage-build](https://docs.docker.com/develop/develop-images/multistage-build/)
