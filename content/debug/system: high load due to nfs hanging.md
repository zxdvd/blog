```metadata
tags: linux,debug,system,load
```

## system: high load due to nfs hanging

Recently, I met a server with high load that I didn't find any significant problem about
 CPU, memory or IO. Write this article to how to find the root cause.

It's a server for our development environment. At first, someone reports that he can't
 access his frontend project.

Then I found it seems that all projects are not available. I think maybe something wrong with
 nginx.

 I logined into the server and found that the nginx processes were alive. And `uptime` indicated
 that the system load is very high, it was 16 (server has 4 CPUs). But result of `top` was ok,
  I didn't find any process that ate too much CPU. I knew that load cound be very high due to
  iowait. But iowait in `top` was about 0.2, very small.

For memory, it only used 8GB of 16GB. And `iostat` showed there isn't any large io.

I tried to curl `http://localhost:80` but it hung. I checked `/proc/pid/status` of a nginx worker
 and found that its state is `D (disk sleep)`. And I used following command to get state of
 all workers and found that all of them were in state `D`.

    $ ps -e -o user,pid,ppid,c,stime,state,time,command | grep -i 1234

I used to try to `strace` the worker process. But I didn't get anything since they were in
 uninterruptable state.

It seems that all workers were hung in io. I want to get info about disks and tried a `df -h`
 but it also hung. Then I tried with `strace df -h` and I got the final result. It hung at
 following line

    stat("/mnt/XXXX"...

The `/mnt/XXXX` is a NFS mount point. And then I found that the nfs is disconnected due to problem
 of the nfs server side.

After fixed problem of the server, everything works well.

### summary
- process will go to state `D` and become uninterruptable when doing io over disconnected nfs
- nfs hanging can lead to high load but `iowait` of top doesn't indicate this
