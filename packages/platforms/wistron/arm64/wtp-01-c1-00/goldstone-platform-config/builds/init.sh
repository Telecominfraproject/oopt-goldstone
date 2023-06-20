#!/bin/sh

set -eux

modprobe fsl-enetc-chardev-mdio
modprobe arm64-wistron-wtp-01-c1-00-cfp2
modprobe arm64-wistron-wtp-01-c1-00-dpll
modprobe arm64-wistron-wtp-01-c1-00-psu
modprobe arm64-wistron-wtp-01-c1-00-led

echo dpll1 0x24 > /sys/bus/i2c/devices/i2c-0/new_device

systemctl enable gs-south-onlp
systemctl enable gs-south-tai
systemctl enable gs-south-system
systemctl enable gs-south-gearbox
systemctl enable gs-south-dpll
systemctl enable gs-xlate-or
