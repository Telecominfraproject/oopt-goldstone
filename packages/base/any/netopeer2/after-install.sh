#!/bin/sh

set -eux

rm -rf /dev/shm/*

/usr/bin/netopeer2.sh install

systemctl enable gs-north-netconf.service
