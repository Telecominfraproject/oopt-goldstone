#!/bin/sh

set -eux

TARGET=`realpath $1`

SONIC_PKGS=`cat $X1/builds/any/rootfs/sonic-packages.yml | python -c 'import yaml, sys; d=yaml.load(sys.stdin.read()); print " ".join("{}:all".format(i) for i in d) if d else ""'`

if [ -z "$SONIC_PKGS" ]; then
    exit 0
fi

cd $X1

for pkg in $SONIC_PKGS; do
    sudo -E -u $SUDO_USER $ONL/tools/onlpm.py --try-arches $ARCH all --build $pkg
    dpkg --root $TARGET -i `$ONL/tools/onlpm.py --lookup $pkg` || true
done

DEBIAN_FRONTEND=noninteractive chroot $TARGET apt install --no-install-recommends -f -qy
