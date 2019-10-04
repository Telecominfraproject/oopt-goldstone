#!/bin/sh

set -eux

docker inspect docker-database:latest || docker load < /var/cache/apt/docker-database.gz
systemctl enable database
rm /var/cache/apt/docker-database.gz
