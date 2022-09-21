#!/bin/sh

set -eux

modprobe cfp2piu

echo piu1 0x6A > /sys/bus/i2c/devices/i2c-41/new_device
echo piu2 0x6A > /sys/bus/i2c/devices/i2c-43/new_device
echo piu3 0x6A > /sys/bus/i2c/devices/i2c-45/new_device
echo piu4 0x6A > /sys/bus/i2c/devices/i2c-47/new_device
echo piu5 0x6A > /sys/bus/i2c/devices/i2c-49/new_device
echo piu6 0x6A > /sys/bus/i2c/devices/i2c-51/new_device
echo piu7 0x6A > /sys/bus/i2c/devices/i2c-53/new_device
echo piu8 0x6A > /sys/bus/i2c/devices/i2c-55/new_device

systemctl disable ipmi
systemctl disable ipmievd

systemctl enable gs-south-onlp
systemctl enable gs-south-tai
systemctl enable gs-south-system
systemctl enable gs-south-sonic
