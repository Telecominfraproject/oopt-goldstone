#!/bin/sh

set -eux

IMAGE=docker-snmp-sv2
docker inspect $IMAGE:latest || docker load < /var/cache/apt/$IMAGE.gz
systemctl enable snmp
rm /var/cache/apt/$IMAGE.gz
