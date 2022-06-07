#!/bin/sh

set -eux

systemctl disable gs-north-snmp.service
systemctl stop gs-north-snmp.service
