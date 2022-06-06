#!/bin/sh

set -eux

systemctl disable gs-system-telemetry-yang.service gs-system-telemetry.service
systemctl stop gs-system-telemetry-yang.service gs-system-telemetry.service
