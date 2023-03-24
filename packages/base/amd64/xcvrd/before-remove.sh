#!/bin/sh

set -eux

systemctl disable xcvrd.service
xcvrd.sh stop || true
