#!/bin/sh

set -eux

systemctl disable usonic
systemctl disable tai
systemctl disable gs-mgmt-xlate.target
systemctl disable gs-mgmt-south.target
systemctl disable gs-north-gnmi
systemctl enable ocnos
