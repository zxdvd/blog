```metadata
tags: benchmark, io, ioping
```

## io-benchmark: ioping
`ioping` is a simple tool with fewer options to monitor IO latency.

Like the network `ping`, it supports set count, interval and request size.
It has options about linux aio, buffered/direct IO, suquential/random IO, read/write.

### a simple sequential read test

``` shell
# ioping  -RL /tmp/
--- /tmp/ (ext4 /dev/vda1) ioping statistics ---
1.28 k requests completed in 2.96 s, 319.2 MiB read, 430 iops, 107.7 MiB/s
generated 1.28 k requests in 3.00 s, 319.5 MiB, 425 iops, 106.5 MiB/s
min/avg/max/mdev = 1.33 ms / 2.32 ms / 34.1 ms / 4.05 ms
```

### random read

``` shell
# ioping -s 64k -R /tmp/

--- /tmp/ (ext4 /dev/vda1) ioping statistics ---
1.86 k requests completed in 2.98 s, 116.6 MiB read, 625 iops, 39.1 MiB/s
generated 1.87 k requests in 3.00 s, 116.6 MiB, 621 iops, 38.9 MiB/s
min/avg/max/mdev = 1.22 ms / 1.60 ms / 45.7 ms / 1.23 ms
```

### write test (large block, direct io)

``` shell
# ioping -D -c 10 -s 10M -W -q .

--- . (ext4 /dev/vdb1) ioping statistics ---
9 requests completed in 381.0 ms, 90 MiB written, 23 iops, 236.2 MiB/s
generated 10 requests in 9.04 s, 100 MiB, 1 iops, 11.1 MiB/s
min/avg/max/mdev = 37.6 ms / 42.3 ms / 55.3 ms / 5.31 ms
```

### references
- [github: ioping](https://github.com/koct9i/ioping/)
