#!/bin/sh

docker inspect docker-orchagent-brcm:latest || docker load < /var/cache/apt/docker-orchagent.gz
rm /var/cache/apt/docker-orchagent.gz
