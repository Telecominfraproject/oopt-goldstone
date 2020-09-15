#!/bin/bash

set -eux

source /etc/profile.d/onl-platform-current.sh # modify $PATH to include onlpdump

if [ -e /etc/goldstone/platform ]; then
    PLATFORM=`cat /etc/goldstone/platform`
elif [ -e /etc/onl/platform ]; then
    PLATFORM=`cat /etc/onl/platform`
    mkdir -p /etc/goldstone
    echo $PLATFORM > /etc/goldstone/platform
else
    PLATFORM=`onlpdump -o | awk '/Platform Name:/{ print $3 }'`
    if [ -z "$PLATFORM" ]; then
        echo "no platform detected by onlpdump command"
        exit 1
    fi
    PLATFORM=${PLATFORM//_/-}
    mkdir -p /etc/goldstone
    echo $PLATFORM > /etc/goldstone/platform
fi

rm -rf /var/lib/goldstone/device/current

if [ ! -d /var/lib/goldstone/device/$PLATFORM ]; then
    echo "/var/lib/goldstone/device/$PLATFORM not found"
    exit 1
fi

cd /var/lib/goldstone/device && ln -sf $PLATFORM current
