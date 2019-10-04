#!/bin/sh

set -eux

IMAGE=docker-orchagent-brcm
docker inspect $IMAGE:latest || docker load < /var/cache/apt/docker-orchagent.gz
systemctl enable swss
rm /var/cache/apt/docker-orchagent.gz
