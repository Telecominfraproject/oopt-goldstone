#!/bin/bash

pushd $SONIC
. functions.sh
popd

sonic_asic_platform=broadcom

MAKEFLAGS="--quiet --no-print-directory"
export MAKEFLAGS

set dummy $(make -C $SONIC -f Makefile.work showtag)
docker_tag=$2

echo "Docker tag is $docker_tag"

set dummy $(make -C $SONIC -f Makefile.work SONIC_BUILD_INSTRUCTION="cat" /etc/debian_version)
ver=$2

echo "sonic debian version is $ver"

# Hm, sonic has its own kernel version but that is not what ONL is running
# it's difficult to predict the ONL kernel version since it is platform-specific
grep = $SONIC/rules/linux-kernel.mk > linux-kernel.mk
kver=$(make -f slave.mk showkernel)
echo "sonic kernel version is $kver"

sudo tee sonic_version.yml > /dev/null <<EOF
build_version: '$SONIC_VERSION'
asic_type: $sonic_asic_platform
commit_id: '$(git rev-parse --short HEAD)'
build_date: $(date -u)
build_number: ${BUILD_NUMBER:-0}
built_by: $USER@$HOSTNAME
debian_version: $ver
kernel_version: $kver
EOF
