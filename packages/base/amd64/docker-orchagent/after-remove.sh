#!/bin/sh

docker inspect docker-orchagent-brcm:latest || docker rmi docker-orchagent-brcm:latest
