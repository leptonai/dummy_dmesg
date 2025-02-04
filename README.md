
# Kernel module for dummy dmesg

This is a kernel module for printing messages in kernel logs.

## How to build

```
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
```

# How to install the module to kernel and make a device file

```
sudo insmod dummy_dmesg.ko
```

then,

```
sudo mknod /dev/dummy_dmesg c <device major number> 0
```

Note that you can find device major number in dmesg after insmod like this:

```
$ sudo dmesg | grep major
[  264.832037] module loaded with device major number 235
```

# How to print message visible from dmesg

```
sudo sh -c "echo \"dummy message, xid 13\" > /dev/dummy_dmesg"
```

You can find message like this:

```
$ sudo dmesg | grep xid
[ 1095.663854] dummy message, xid 13
```
