#!/bin/sh

set -eux

systemctl disable tai-gearbox.service
systemctl stop tai-gearbox.service
