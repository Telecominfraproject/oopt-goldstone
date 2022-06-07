#!/bin/sh

set -eux

groupadd -f gsmgmt

systemctl enable gs-yang.service
