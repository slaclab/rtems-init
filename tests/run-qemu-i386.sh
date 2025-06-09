#!/usr/bin/env bash

set -e
cd "$(dirname "${BASH_SOURCE[0]}")/../"

KERNEL="build-cmake/build-rtems7-pc686-qemu/rtems-init.exe"

. tests/conf.local.sh

qemu-system-i386 -m 512 -no-reboot -serial mon:stdio -nographic -net nic,model=e1000 \
    -net user \
    -append "--video=off --console=/dev/com1 --mount=10.0.2.2:$EPICS_TOP:/epics --cwd=/home/jeremy/dev/epics/ioc/atlas/bin/RTEMS-pc686" \
    $ARGS -kernel "$KERNEL" $@
