#!/bin/sh

set -eux

systemctl disable gs-xlate-oc.service
systemctl stop gs-xlate-oc.service
