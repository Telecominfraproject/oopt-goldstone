#!/bin/bash

set -eux

kubectl create configmap usonic-config --from-file="/var/lib/goldstone/device/current/usonic" --dry-run=client -o yaml | kubectl apply -f -
