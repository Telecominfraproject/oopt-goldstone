#!/bin/bash

pushd $SONIC
. functions.sh
popd

sonic_asic_platform=broadcom

sudo tee sonic_version.yml > /dev/null <<EOF
build_version: '$SONIC_VERSION'
asic_type: $sonic_asic_platform
commit_id: '$(git rev-parse --short HEAD)'
build_date: $(date -u)
build_number: ${BUILD_NUMBER:-0}
built_by: $USER@$BUILD_HOSTNAME
EOF
