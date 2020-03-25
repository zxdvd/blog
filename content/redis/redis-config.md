<!---
tags: redis
-->

## redis config

### config from command line
You can also add options from command line. Just add `--` before the keyword.

    $ ./redis-server  --port 6000

### online config
Some configs support online update. You can use `config get xxx` to get it and use
`config set xxx` to set it. Attention, this doesn't update the redis.conf file so
 it won't take effect after restart.

### config overwrite
You can set same config for multiple times. However, only the last one will take effect.
It's easy to understand for simple config like `port`, `timeout`. However, for `save`,
 it allows multiple line config like

```
save 900 1               # save if there has at least 1 key changed in 900 seconds
save 60 10000            # save if there has at least 10000 keys changed in 60 seconds
```

The `save 60 10000` won't overwrite the previous `save 900 1`. All of the save will take
 effect. Then how to overwrite it?

You can just add a line `save ""` and it means that all previous `save` were discarded.

```
save ""                  # discard all previous save
save 60 3000             # new settings
save 300 10
```

### references
[redis.io: config](https://redis.io/topics/config)
