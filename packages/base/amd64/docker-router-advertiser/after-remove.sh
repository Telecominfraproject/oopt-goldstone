#!/bin/sh

docker inspect docker-router-advertiser:latest || docker rmi docker-router-advertiser:latest
systemctl disable radv
