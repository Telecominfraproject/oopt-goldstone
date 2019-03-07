#!/bin/sh

docker inspect docker-snmp-sv2:latest || docker rmi docker-snmp-sv2:latest
systemctl disable snmp
