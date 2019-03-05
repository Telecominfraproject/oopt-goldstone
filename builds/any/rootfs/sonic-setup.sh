#!/bin/bash

# this script will be executed after multistrap to add the sonic repo
# adding sonic-dev repo in multistrap stage somehow raises the following issue
# https://blog.packagecloud.io/eng/2016/03/21/apt-hash-sum-mismatch/

set -xeu

FILESYSTEM_ROOT=`realpath $1`

cd $SONIC

. functions.sh
IMAGE_CONFIGS=files/image_config

cp $IMAGE_CONFIGS/apt/sources.list.d/packages_microsoft_com_repos_sonic_dev.list $FILESYSTEM_ROOT/etc/apt/sources.list.d/
cat $IMAGE_CONFIGS/apt/sonic-dev.gpg.key | sudo LANG=C chroot $FILESYSTEM_ROOT apt-key add -

DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT apt-get update

DEBIAN_FRONTEND=noninteractive chroot $FILESYSTEM_ROOT apt-get -y install python-click-default-group python-tabulate python-natsort
# get these packages once sonic-dev is enabled (after initial multistrap)

# missing files from the sonic installer
# (see # sonic-buildimage/files/build_templates/sonic_debian_extension.j2)
mkdir -p $FILESYSTEM_ROOT/var/cache/sonic
