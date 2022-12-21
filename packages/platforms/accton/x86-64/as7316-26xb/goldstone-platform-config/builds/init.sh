#!/bin/sh

set -eux

systemctl disable usonic
systemctl disable tai
systemctl disbale gs-mgmt.target
systemctl disable gs-mgmt-xlate.target
systemctl disable gs-mgmt-south.target
systemctl disable gs-mgmt-north.target
systemctl enable gs-mgmt-north-netconf
systemctl enable gs-mgmt-notif
