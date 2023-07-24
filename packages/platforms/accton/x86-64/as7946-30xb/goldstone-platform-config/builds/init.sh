#!/bin/sh

set -eux

systemctl disable usonic
systemctl disable tai
systemctl disable gs-mgmt-xlate.target
systemctl enable gs-mgmt-south.target
systemctl disable gs-north-gnmi
systemctl enable ocnos
systemctl enable gs-south-onlp
