```metadata
tags: storage, filesystem, lvm
```

## LVM usage

LVM (Logical Volume Manager) is a tool to manage storage resources. You can create a
 storage pool (volume group) composed by multiple disks or partitions. And then create
 logical volume from this pool and use it as normal partition.

### why LVM
LVM is a middle layer between physical device and file system. Why and when we need this
 abstraction layer?

- compose a large pool via many small devices
- easy to extend vg size and lv size

### how to
For creating with real disks or partitions, just go from Step 1. Step0 is used to setup
 a test environment.

#### Step 0: setup test environment
Create device file.

    # dd if=/dev/zero of=./devices/vda.img count=1000 bs=1MB

Then we need to create a loopback device from the device file. LVM won't use image file
 directly.

    # losetup -f ./devices/vda.img
    # losetup -a                         # list all loopback devices

We can use the loopback device directly or create partition and use partion in LVM.

    # parted /dev/loop0 print           # show disk and partition info
    # parted /dev/loop0 mklabel gpt     # erase all and initialize a GPT partition table
    # parted --align optimal /dev/loop0 mkpart primary 0% 100%    # create a primary partition use whole disk

Iniatialize a physical volume. This is optional since `vgcreate` will do it if not initialized.

    # pvcreate /dev/loop0p1             # optional
    # pvdisplay                         # show PVs

#### Step 1: create volume group
Create volume group:

    # vgcreate vg1 /dev/loop0p1         # create a vg named vg1
    # vgdisplay                         # show VGs

You cannot add an added pv to another vg. You'll get error.

    # vgcreate vg2 /dev/loop0p1
    Physical volume '/dev/loop0p1' is already in volume group 'vg1'
    Unable to add physical volume '/dev/loop0p1' to volume group 'vg1'

#### Step 2: create logical volume
You need to specify a size using `-L 10G` or `-l 1000` (extents). You'll fail if specifying
 a size large than capacity of vg. You can use `-n lv1` to specify the name of the lv.

    # lvcreate -L 100M --name lv1 vg1
    # lvdisplay                         # show LVs

Then you got a device `/dev/vg1/lvol0`.

#### Step 3: create a file system
Then we can create file system on the lv just like normal partition.

    # mkfs -t ext4 /dev/vg1/lvol0                  # create ext4 file system
    # mount -t ext4 /dev/vg1/lvol0 /mnt/tmp/l1     # mount it
    # echo hello > /mnt/tmp/l1/world.txt           # add a file

### resize
We can extend or shrink the lv. `lvresize` = `lvextend` + `lvreduce`.

    # lvresize --resizefs -l 40 /dev/vg1/vol0                 # resize to 40 extents

The `--resizefs` is very important here, you need to resize file system so that extended
 space can be used by file system.

For shrinking, you will try to shrink file system first. If it failed, the `lvresize`
 will stop. You can do force resize but the file system may be broken.

### raid logical volume
You can create a raid lv. It uses `md/mdadm` underground to achieve soft raid. At least
 two PVs are needed for raid1.

```
    # losetup -f ./devices/vdb.img               # create another loopback device
    # vgextend vg1 /dev/loop1                    # add the whole disk to vg1

    # lvcreate --mirrors 1 --type raid1 -l 10 -n lvraid1_1 vg1
    # lvdisplay                         # show LVs
```

It also supports raid4, raid5, raid10.


### references
- [arch wiki: lvm on soft raid](https://wiki.archlinux.org/index.php/LVM_on_software_RAID)
