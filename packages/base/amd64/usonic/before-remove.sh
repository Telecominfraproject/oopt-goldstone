#!/bin/sh

set -eux

systemctl disable load-usonic-config.service
kubectl delete -f /var/lib/rancher/k3s/server/manifests/usonic/ || true
