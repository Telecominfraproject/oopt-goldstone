#!/bin/sh

docker inspect docker-fpm-frr:latest || docker rmi docker-fpm-frr:latest
systemctl disable bgp
