#!/bin/sh

set -eux

# release reset states of QSFP modules
echo 0 | tee $(seq -f "/sys/devices/pci0000:00/0000:00:1f.3/i2c-0/i2c-1/i2c-12/12-0061/module_reset_%g" 1 16) 1> /dev/null
echo 0 | tee $(seq -f "/sys/devices/pci0000:00/0000:00:1f.3/i2c-0/i2c-1/i2c-13/13-0062/module_reset_%g" 17 26) 1> /dev/null

systemctl disable usonic
systemctl disable tai
systemctl disable gs-mgmt-xlate.target
systemctl enable gs-mgmt-south.target
systemctl disable gs-north-gnmi
systemctl enable ocnos
systemctl enable gs-south-onlp
systemctl enable gs-south-ocnos
systemctl enable xcvrd
