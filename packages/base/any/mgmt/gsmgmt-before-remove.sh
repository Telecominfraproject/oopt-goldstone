#!/bin/sh

set -eux

systemctl disable gs-mgmt.target gs-mgmt-south.target gs-mgmt-north.target gs-mgmt-xlate.target gs-mgmt-system.target gs-north-notif.service
systemctl stop gs-mgmt.target gs-mgmt-south.target gs-mgmt-north.target gs-mgmt-xlate.target gs-mgmt-system.target gs-north-notif.service
