```metadata
tags: linux, ipc, memory
```

## ipc: shared memory
Linux and most unix supports both system V shared memory and POSIX shared memory.
The POSIX is always suggested.

### POSIX interface

```c
 #include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

int shm_open(const char *name, int oflag, mode_t mode);    // open a file descriptor via file path
int ftruncate(int fd, off_t length);                       // extend fd to requested size in mmap
void *mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset);                   // mmap into process's virtual address space
close(fd);                           // you can close the fd returned by shm_open now
// use the shared memory

// destroy and clean
int munmap(void *addr, size_t length);                     // unmap, oppose to the mmap
int shm_unlink(const char *name);                          // unlink shared memory, oppose to the shm_open
```

Attention:

- the `ftruncate` is used to extend the the file so that it has enough size to mmap
- you need to link with `-lrt`.

### system V interface

```c
 #include <sys/ipc.h>
#include <sys/shm.h>

int shmget(key_t key, size_t size, int shmflg);        // just like shm_open, need a key not a name, and size is set here
void *shmat(int shmid, const void *shmaddr, int shmflg);    // attach to process's address space
int shmdt(const void *shmaddr);                        // just like munmap
int shmctl(int shmid, int cmd, struct shmid_ds *buf);  // use `IPC_RMID` as cmd to delete, like shm_unlink
```

If you like the path name style used in shm_open, you can use the `ftok` to convert
a path to a key.

### difference between POSIX shared memory and system V shared memory

- The POSIX use a path that starts with `/`, and you can see all created under `/dev/shm`.
  But the system V one use a integer as key, you can get all created using `ipcs` commands.
  Sometimes path name is easy to shared between processes than key.

- The POSIX shm_open and shm_unlink are similar to open and unlink. You can treat with as
  a normal file descriptor.


### excellent interface that wraps different shared memory implementations
Postgresql has a excellent shared memory interface that supports POSIX, system V and windows.
It exposes a simple function that supports 4 operations.

```c
// postgres/src/include/storage/dsm_impl.h
typedef enum
{
	DSM_OP_CREATE,
	DSM_OP_ATTACH,
	DSM_OP_DETACH,
	DSM_OP_DESTROY
} dsm_op;

/* Create, attach to, detach from, resize, or destroy a segment. */
extern bool dsm_impl_op(dsm_op op, dsm_handle handle, Size request_size,
						void **impl_private, void **mapped_address, Size *mapped_size,
						int elevel);
```

Detailed implementations at [here](https://github.com/postgres/postgres/blob/master/src/backend/storage/ipc/dsm_impl.c).

### shared memory vs anonymous mmap
You can create a shared anonymous mmap area and then fork. Then the child process and
 parent process share this same area. Almost same as the POSIX/system V shared memory.
However, it is only shared between parent and the forked child. You cannot use it to
 share with any other program.

With shared memory, you can shared with ANY program on the same host.

Another side, you may get OOM due to memory over commit when using mmap. But you won't
 get it with shared memory.

And mmap memory will be released if process crashed or exited while shared memory will
 be kept. You can re-attach shared memory after restarting the program.

### Attentions
- For POSIX shared memory, you need to link with `-lrt` (librt.so, the realtime library).
- Created shared memory won't be removed if the process exited without unlink it.
