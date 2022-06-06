#!/bin/sh

set -eux

systemctl enable gs-system-telemetry-yang.service gs-system-telemetry.service
