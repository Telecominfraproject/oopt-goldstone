#!/bin/sh

docker inspect docker-syncd-brcm:latest || docker load < /var/cache/apt/docker-syncd.gz
rm /var/cache/apt/docker-syncd.gz
