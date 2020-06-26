#!/bin/sh

set -eux

systemctl daemon-reload
systemctl enable load-usonic-config.service

mkdir -p /var/lib/usonic/redis
