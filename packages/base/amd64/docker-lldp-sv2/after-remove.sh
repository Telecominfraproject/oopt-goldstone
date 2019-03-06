#!/bin/sh

docker inspect docker-lldp-sv2:latest || docker rmi docker-lldp-sv2:latest
systemctl disable lldp
