#!/bin/bash

set -eux

if [ -e /etc/goldstone/platform]; then
    PLATFORM=`cat /etc/goldstone/platform`
else
    PLATFORM=`onie-sysinfo`
    PLATFORM=${PLATFORM//_/-}
    mkdir -p /etc/goldstone
    echo $PLATFORM > /etc/goldstone/platform
fi

cd /var/lib/goldstone/device && ln -sf $PLATFORM current
