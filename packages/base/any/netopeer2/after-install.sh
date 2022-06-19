#!/bin/sh

set -eux

rm -rf /dev/shm/*

systemctl enable gs-north-netconf.service
