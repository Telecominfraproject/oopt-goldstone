#!/bin/sh

docker inspect docker-fpm-quagga:latest || docker load < /var/cache/apt/docker-fpm-quagga.gz
systemctl enable bgp
rm /var/cache/apt/docker-fpm-quagga.gz
