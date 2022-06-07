#!/bin/sh

set -eux

systemctl enable gs-mgmt.target gs-mgmt-south.target gs-mgmt-north.target gs-mgmt-xlate.target gs-north-notif.service
