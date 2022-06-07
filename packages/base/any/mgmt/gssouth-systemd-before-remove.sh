#!/bin/sh

set -eux

systemctl disable gs-south-system.service

python -m pip uninstall --yes gssystem
