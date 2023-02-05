#!/bin/sh

set -eux

systemctl disable ocnos.service
ocnos.sh stop || true
