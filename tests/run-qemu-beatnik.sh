#!/usr/bin/env bash

set -e
cd "$(dirname "${BASH_SOURCE[0]}")/../"

KERNEL="build-cmake/build-rtems6-beatnik/rtems-init"

qemu-system-ppc -m 64 -no-reboot -serial mon:stdio -nographic -net nic,model=e1000 \
    -net user,restrict=yes -append "--video=off --console=/dev/com1" -cpu 7457 -M mac99 $ARGS -s -S -kernel "$KERNEL"
