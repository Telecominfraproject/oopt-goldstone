#!/bin/sh

docker inspect docker-syncd-brcm:latest || docker rmi docker-syncd-brcm:latest
