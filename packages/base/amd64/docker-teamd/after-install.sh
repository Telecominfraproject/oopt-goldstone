#!/bin/sh

docker inspect docker-teamd:latest || docker load < /var/cache/apt/docker-teamd.gz
systemctl enable teamd
rm /var/cache/apt/docker-teamd.gz
