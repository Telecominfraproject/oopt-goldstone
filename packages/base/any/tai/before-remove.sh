#!/bin/sh

set -eux

systemctl disable tai.service
systemctl stop tai.service
