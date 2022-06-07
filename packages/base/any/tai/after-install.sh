#!/bin/sh

set -eux

systemctl enable tai.service
ldconfig -n /var/lib/tai/lib
