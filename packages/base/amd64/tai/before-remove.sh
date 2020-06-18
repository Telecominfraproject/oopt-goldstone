#!/bin/sh

set -eux

kubectl delete -f /var/lib/rancher/k3s/server/manifests/tai/ || true
