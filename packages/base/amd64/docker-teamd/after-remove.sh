#!/bin/sh

docker inspect docker-teamd:latest || docker rmi docker-teamd:latest
systemctl disable teamd
