# device mapper proxy

Simple device mapper that propagate io operations to mapped device and count requests and averarge block size (both for write and read operations).

Results are available through `sysfs`.

## Build

To build module just use `make`:

```sh
$ make
```

To clean artifacts:

```sh
$ make clean
```

## Usage

Insert a module into a kernel:

```sh
# insmod dmp.ko
```

Create test block device:

```sh
# dmsetup create $test1 --table "0 512 zero"
```

Create proxy device (provided by this module):

```sh
# dmsetup create dmp1 --table "0 512 dmp /dev/mapper/$test1"
```

Some read/write operations: 

```sh
# dd if=/dev/random of=/dev/mapper/dmp1 bs=4k count=1
# dd of=/dev/null if=/dev/mapper/dmp1 bs=4k count=1
```

To see statistics, use 

```sh
$ cat /sys/module/dmp/$test1/stat
```

