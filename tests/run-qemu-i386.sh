#!/usr/bin/env bash

set -e
cd "$(dirname "${BASH_SOURCE[0]}")/../"

KERNEL="build-cmake/build-rtems6-pc686-qemu/rtems-init"

qemu-system-i386 -m 64 -no-reboot -serial mon:stdio -nographic -net nic,model=e1000 -net user,restrict=yes -append "--video=off --nodhcp --console=/dev/com1" $ARGS -kernel "$KERNEL"
