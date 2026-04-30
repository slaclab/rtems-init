# RootFS

With the introduction of libbsd, rtems-net-services and other BSD ports, the number of configuration files required
to provision an RTEMS system has grown quite a bit. The original approach was to create these files on the fly in C using
standard POSIX syscalls (open/write/close), however can be pretty difficult to maintain over time.

rtems-init implements a simple rootfs system that mirrors something you'd find with Buildroot (the Linux build system).

Internally, the rootfs is just a tarball that RTEMS unpacks at runtime into the in-memory FS (IMFS). This tarball
is generated at compile time by a Python script in the `tools` submodule, and embedded into the binary using
a `.incbin` assembler directive.

The rootfs for rtems-init can be found in the `rootfs` subdirectory.

## Adding Files

New files should be added under the `rootfs` subdirectory, generally in the same layout as they will be in the destination
file system.

Files to be built into the rootfs must be listed in `rootfs/rootfs.txt`. The format of this file is as follows:
```
# file                          dest     permissions   owner:group
etc/passwd                      /        0644
etc/group                       /        0644
etc/dhcpcd.conf                 /        0644
```

`owner:group` is optional and specifies the owning uid/gid (a numeric value!).