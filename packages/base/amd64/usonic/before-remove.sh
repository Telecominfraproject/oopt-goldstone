#!/bin/sh

set -eux

systemctl disable usonic.service
usonic.sh stop || true
