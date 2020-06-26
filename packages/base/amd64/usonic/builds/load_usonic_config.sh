#!/bin/bash

set -eux

if [ -e /etc/usonic/USONIC_PLATFORM ]; then
    PLATFORM=`cat /etc/usonic/USONIC_PLATFORM`
else
    PLATFORM=`onie-sysinfo`
    PLATFORM=${PLATFORM//_/-}
    mkdir -p /etc/usonic
    echo $PLATFORM > /etc/usonic/USONIC_PLATFORM
fi

kubectl create configmap usonic-config --from-file="/var/lib/goldstone/device/$PLATFORM" --dry-run=client -o yaml | kubectl apply -f -
