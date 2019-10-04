#!/bin/sh

set -eux

IMAGE=docker-teamd
docker inspect $IMAGE:latest || docker load < /var/cache/apt/$IMAGE.gz
systemctl enable teamd
rm /var/cache/apt/$IMAGE.gz
