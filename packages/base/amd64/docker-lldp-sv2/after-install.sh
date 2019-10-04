#!/bin/sh

set -eux

IMAGE=docker-lldp-sv2
docker inspect $IMAGE:latest || docker load < /var/cache/apt/$IMAGE.gz
systemctl enable lldp
rm /var/cache/apt/$IMAGE.gz
