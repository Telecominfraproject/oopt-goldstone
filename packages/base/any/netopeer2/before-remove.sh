#!/bin/sh

set -eux

systemctl disable gs-north-netconf.service
netopeer2.sh stop || true

/usr/bin/netopeer2.sh remove
