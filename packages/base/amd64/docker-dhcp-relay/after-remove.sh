#!/bin/sh

set -eux

docker inspect docker-dhcp-relay:latest || docker rmi docker-dhcp-relay:latest
systemctl disable dhcp_relay
