```metadata
tags: benchmark, io, fio
```

## io-benchmark: fio

### key options

#### size
Total IO size of each single job. If you set `size` to 100M, `numjobs` to 4, the total
 IO size will be 100M * 4.

#### direct
It will use non-buffered IO (O_DIRECT flag) if this is true. It is false
by default which means buffered IO is used.

#### readwrite
Type of IO to test, default `read`. It supports following types:
    - read / randread
    - write / randwrite
    - trim / randtrim
    - rw,readwrite (sequential mixed reads and writes)
    - randrw (random mixed reads and writes)
    - trimwrite

#### ioengine
Systemcalls used for the IO test, default `psync`. Support following types:
    - sync: basic read or write, lseek
    - psync / vsync / pvsync
    - io_uring: latest asynchronous IO of linux, supports both direct and buffered IO
    - libaio: asynchronous IO of linux, only supports direct IO
    - mmap: mmap IO
    - rdma

    It supports many other types but not frequently used, so I don't list them all.

### filename
If you have multiple disks. You can use filename to specify a disk, like
 `filename=/mnt/sda/testfile`. Don't use `file=/dev/sda` if the disk has data.

#### numjobs
Set how many concurrent jobs to run.

### how to
You can simply run `fio` with all options in command line, like following

    # fio --size=100M --readwrite=read --direct=1 --name=job1

For complex jobs, you can write them in a config file, like following

```
[global]
size=256M

[job1]
ioengine=libaio
iodepth=4
rw=randwrite
numjobs=4

[job2]
rw=randread
numjobs=2
```

Then you can execute it using `fio fio.cfg`. You can choose only one job to run using
 `fio --section=job2 fio.cfg`.

### references
- [fio official doc](https://fio.readthedocs.io/en/latest/fio_doc.html)
