#!/bin/sh

set -eux

systemctl disable gs-north-gnmi.service
systemctl stop gs-north-gnmi.service
