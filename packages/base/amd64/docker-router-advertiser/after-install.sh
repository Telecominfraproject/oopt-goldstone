#!/bin/sh

set -eux

IMAGE=docker-router-advertiser
docker inspect $IMAGE:latest || docker load < /var/cache/apt/$IMAGE.gz
systemctl enable radv
rm /var/cache/apt/$IMAGE.gz
