#!/bin/sh

set -eux

systemctl disable gs-xlate-or.service
systemctl stop gs-xlate-or.service
