#!/bin/sh

docker inspect docker-snmp-sv2:latest || docker load < /var/cache/apt/docker-snmp-sv2.gz
systemctl enable snmp
rm /var/cache/apt/docker-snmp-sv2.gz
