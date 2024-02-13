# device mapper proxy

Simple device mapper that propagate io operations to mapped device and count requests and averarge block size (both for write and read operations).

Results are available through `sysfs`.

## Build

To build module just use `make`:

```console
$ make
```

To clean artifacts:

```console
$ make clean
```

## Usage

Insert a module into a kernel:

```console
# insmod dmp.ko
```

Create test block device:

```console
# dmsetup create $test1 --table "0 512 zero"
```

Create proxy device (provided by this module):

```console
# dmsetup create dmp1 --table "0 512 dmp /dev/mapper/$test1"
```

Some read/write operations: 

```console
# dd if=/dev/random of=/dev/mapper/dmp1 bs=4k count=1
# dd of=/dev/null if=/dev/mapper/dmp1 bs=4k count=1
```

To see statistics, use 

```console
$ cat /sys/module/dmp/$test1/stat
```

> note: since we are interested in underlying device and creating more than one proxy for device is forbidden, sysfs folder named by underlying device.

## Testing

Run [init script](init.sh) with root privileges which create one device (`zero1`) with proxy device (`dmp0`) and one device (`zero2`) with one proxy device (`dmp2`) and proxy device to itself (`dmp3` -> `dmp2`).

Now you can do different read/write operations and check results using 
```console 
$ cat /sys/module/dmp/$dev/stat
```