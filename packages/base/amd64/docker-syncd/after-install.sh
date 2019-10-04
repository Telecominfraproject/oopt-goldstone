#!/bin/sh

set -eux

IMAGE=docker-syncd-brcm:latest
docker inspect $IMAGE || docker load < /var/cache/apt/docker-syncd.gz
systemctl enable syncd
rm /var/cache/apt/docker-syncd.gz
