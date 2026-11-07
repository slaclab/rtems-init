#!/usr/bin/env bash

set -e
cd "$(dirname "${BASH_SOURCE[0]}")/../"

TARGET=i386-rtems7-pc686
while test $# -gt 0; do
    case $1 in
    -t)
        TARGET="$2"
        shift
        ;;
    -a)
        KARGS="$KARGS $2"
        shift
        ;;
    -b)
        BUILD=1
        ;;
    --gdb)
        QEMUARGS="$QEMUARGS -s -S"
        ;;
    esac
    shift
done

if [ "${BUILD}"x = "1x" ]; then
    # Build first
    make -C build/${TARGET}
fi

KERNEL="build/${TARGET}/rtems-init.exe"

. tests/conf.local.sh

qemu-system-i386 -m 128 -no-reboot -serial mon:stdio -nographic \
    -device e1000,netdev=em0 -netdev user,id=em0,hostfwd=tcp::10004-:10003,hostfwd=tcp::1234-:1234,\
hostfwd=tcp::5064-:5064,hostfwd=udp::5065-:5065,hostfwd=tcp::5075-:5075,hostfwd=udp::5076-:5076 \
    $QEMUARGS \
    -append "$KARGS --video=off --console=/dev/com1 --mount=9p::$(id -u).$(id -g)@10.0.2.2!10003:$RTEMS_TOP:/rtems --cwd=$RTEMS_TOP --netconfig=em0,10.0.2.15,255.255.255.0" \
    $ARGS -kernel "$KERNEL" $@
