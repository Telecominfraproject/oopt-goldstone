#!/bin/sh

docker inspect docker-fpm-frr:latest || docker load < /var/cache/apt/docker-fpm-frr.gz
systemctl enable bgp
rm /var/cache/apt/docker-fpm-frr.gz
