#!/bin/sh

docker inspect docker-router-advertiser:latest || docker load < /var/cache/apt/docker-router-advertiser.gz
systemctl enable radv
rm /var/cache/apt/docker-router-advertiser.gz
