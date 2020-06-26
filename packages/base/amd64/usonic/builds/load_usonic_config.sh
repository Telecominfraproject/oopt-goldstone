#!/bin/bash

set -eux

PLATFORM=`onie-sysinfo`
PLATFORM=${PLATFORM=//_/-}

kubectl create configmap usonic-config --from-file="/var/lib/usonic/device/$PLATFORM" --dry-run=client -o yaml | kubectl apply -f -
