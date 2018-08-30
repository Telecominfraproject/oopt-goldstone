#!/bin/sh

set -eu

TARGET=`realpath $1`

DOCKER_PKGS=`cat $X1/builds/any/rootfs/amd64-docker-packages.yml | python -c 'import yaml, sys; d=yaml.load(sys.stdin.read()); print " ".join("{}:amd64".format(i) for i in d) if d else ""'`

if [ -z "$DOCKER_PKGS" ]; then
    exit 0
fi

cd $X1

for pkg in $DOCKER_PKGS; do
    sudo -E -u $SUDO_USER $ONL/tools/onlpm.py --try-arches $ARCH all --build $pkg
    dpkg --root $TARGET --unpack `$ONL/tools/onlpm.py --lookup $pkg`
done

mkdir /newroot
rm -r $TARGET/oldroot 2> /dev/null || true
mkdir $TARGET/oldroot
mount --bind $TARGET /newroot
cd /newroot
pivot_root . oldroot

mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev

rm -rf /var/run/docker*

service docker start

sleep 5

dpkg --configure -a

cd /oldroot
pivot_root . newroot

umount -R -l /newroot
