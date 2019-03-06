#!/bin/sh

docker inspect docker-fpm-quagga:latest || docker rmi docker-fpm-quagga:latest
systemctl disable bgp
