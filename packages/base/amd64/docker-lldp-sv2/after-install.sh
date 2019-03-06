#!/bin/sh

docker inspect docker-lldp-sv2:latest || docker load < /var/cache/apt/docker-lldp-sv2.gz
systemctl enable lldp
rm /var/cache/apt/docker-lldp-sv2.gz
