#!/bin/sh

docker inspect docker-database:latest || docker rmi docker-database:latest
systemctl disable database
