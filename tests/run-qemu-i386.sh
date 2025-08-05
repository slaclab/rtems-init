#!/usr/bin/env bash

set -e
cd "$(dirname "${BASH_SOURCE[0]}")/../"

while test $# -gt 0; do
    case $1 in
    -a)
        KARGS="$KARGS $2"
        shift
        ;;
    --gdb)
        QEMUARGS="$QEMUARGS -s -S"
        ;;
    esac
    shift
done

KERNEL="build-cmake/build-rtems7-pc686-qemu/rtems-init.exe"

. tests/conf.local.sh

qemu-system-i386 -m 128 -no-reboot -serial mon:stdio -nographic \
    -device e1000,netdev=em0 -netdev user,id=em0,hostfwd=tcp::10003-:10003,hostfwd=tcp::1234-:1234,hostfwd=tcp::5512-:5512 \
    $QEMUARGS \
    -append "$KARGS --video=off --console=/dev/com1 --mount=10.0.2.2:$RTEMS_TOP:/rtems --cwd=$RTEMS_TOP" \
    $ARGS -kernel "$KERNEL" $@
