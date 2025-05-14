#!/usr/bin/env bash

cd "$(dirname "${BASH_SOURCE[0]}")"

if [ -z "$RTEMS_TOP" ]; then
    echo "Source your env.fish before continuing"
    exit 1
fi

./waf configure --rtems-tools="$RTEMS_TOP/host/linux-x86_64" --rtems="$RTEMS_TOP/target/rtems" --prefix="$RTEMS_TOP/target/rtems" $@
