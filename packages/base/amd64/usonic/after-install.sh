#!/bin/sh

set -eux

systemctl enable load-usonic-config.service

mkdir -p /var/lib/usonic/redis
