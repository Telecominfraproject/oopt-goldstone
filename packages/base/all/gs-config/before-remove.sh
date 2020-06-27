#!/bin/sh

set -eux

systemctl disable gs-config.service
rm -f /var/lib/goldstone/device/current /etc/goldstone/platform
