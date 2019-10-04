#!/bin/sh

set -eux

docker inspect docker-dhcp-relay:latest || docker load < /var/cache/apt/docker-dhcp-relay.gz
systemctl enable dhcp_relay
rm /var/cache/apt/docker-dhcp-relay.gz
