```metadata
tags: linux, fs, syscall
```

## linux: go through the read syscall
The kernel code referenced in this article is from version 5.8.

First you can use `ag 'SYSCALL_DEFINE.\(read,'` to search the kernel source code to
 get the definition of the read syscall. Syscall are defined like following.

```c
// kernel: fs/read_write.c

SYSCALL_DEFINE3(read, unsigned int, fd, char __user *, buf, size_t, count)
{
	return ksys_read(fd, buf, count);
}
```

The `3` here means that `read` accept 3 paramters. So `SYSCALL_DEFINE0` defines a syscall
 with no paramters.

    SYSCALL_DEFINE0(getpid)

The `ksys_read` just calls `vfs_read`.

```c
// kernel: fs/read_write.c

ssize_t ksys_read(unsigned int fd, char __user *buf, size_t count)
{
	struct fd f = fdget_pos(fd);
	ssize_t ret = -EBADF;

	if (f.file) {
		loff_t pos, *ppos = file_ppos(f.file);
		if (ppos) {
			pos = *ppos;
			ppos = &pos;
		}
		ret = vfs_read(f.file, buf, count, ppos);
		if (ret >= 0 && ppos)
			f.file->f_pos = pos;
		fdput_pos(f);
	}
	return ret;
}
```

The virtual files ystem is a high level layer that defines a group of IO apis and any
 file system can implement them and register itself so that userspace can use this
 file  system via same common apis (read, write).

```c
// kernel: fs/read_write.c

ssize_t vfs_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	ssize_t ret;
    // ignore some mode and length check

	if (file->f_op->read)
		ret = file->f_op->read(file, buf, count, pos);
	else if (file->f_op->read_iter)
		ret = new_sync_read(file, buf, count, pos);
	else
		ret = -EINVAL;
	if (ret > 0) {
		fsnotify_access(file);
		add_rchar(current, ret);
	}
	inc_syscr(current);
	return ret;
}
```

The `vfs_read` uses the `file->f_op->read` or `new_sync_read` to handle the reading. And
 if successful, it will call the `fsnotify` hook and update some counters (read bytes of
 current task and read syscall count of current task).

The `new_sync_read` will initiate a `kiocb` struct and call `file->f_op->read_iter`. So
let's focus on `f_op->read` and `f_op->read_iter`.

The `struct file_operations` is an union interface defined for virtual fie system. Each
 file system has its own implementation (no need to implement all functions). And each
 `struct file` has a `file_operations` pointer `f_op`.

```c
// kernel: include/linux/fs.h

struct file {
    ...
	struct path		f_path;
	struct inode		*f_inode;	/* cached value */
	const struct file_operations	*f_op;
    ...
} __randomize_layout
  __attribute__((aligned(4)));	/* lest something weird decides that 2 is OK */

struct file_operations {
	struct module *owner;
	loff_t (*llseek) (struct file *, loff_t, int);
	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	ssize_t (*read_iter) (struct kiocb *, struct iov_iter *);
	int (*iopoll)(struct kiocb *kiocb, bool spin);
	__poll_t (*poll) (struct file *, struct poll_table_struct *);
	int (*mmap) (struct file *, struct vm_area_struct *);
	unsigned long mmap_supported_flags;
	int (*open) (struct inode *, struct file *);
	int (*flush) (struct file *, fl_owner_t id);
    ...
} __randomize_layout;
```

### ext4 as example
There are so many file systems. I'll use ext4 as example.

```c
// kernel: fs/ext4/file.c

const struct file_operations ext4_file_operations = {
	.llseek		= ext4_llseek,
	.read_iter	= ext4_file_read_iter,
	.write_iter	= ext4_file_write_iter,
	.iopoll		= iomap_dio_iopoll,
    ...
	.mmap		= ext4_file_mmap,
	.mmap_supported_flags = MAP_SYNC,
    ...
};
```

Above is the `file_operations` of ext4, it doesn't define a `read` field but a `read_iter`.
So for ext4 file system, the `vfs_read` will use `ext4_file_read_iter` to do the reading.

```c
// kernel: fs/ext4/file.c

static ssize_t ext4_file_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
    ...
	if (iocb->ki_flags & IOCB_DIRECT)
		return ext4_dio_read_iter(iocb, to);

	return generic_file_read_iter(iocb, to);
}
```

For direct io, it will use `ext4_dio_read_iter`. Otherwise it calls `generic_file_read_iter`
 which calls `generic_file_buffered_read` to do buffered reading.

`generic_file_buffered_read` is a large and complex function that try to use the page
 buffer. I'm not familar with this currently. I'll try to dig into this later.
