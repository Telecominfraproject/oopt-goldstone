#!/bin/sh

set -eu

TARGET=`realpath $1`

DOCKER_PKGS=`cat $X1/builds/any/rootfs/amd64-docker-packages.yml | python -c 'import yaml, sys; d=yaml.load(sys.stdin.read()); print " ".join("{}:amd64".format(i) for i in d) if d else ""'`

if [ -z "$DOCKER_PKGS" ]; then
    exit 0
fi

# emulate steps in $(installer_extra_files) (slave.mk, files/build_templates/sonic_debian_extension.j2)
cp $SONIC/dockers/docker-database/base_image_files/redis-cli $TARGET/usr/bin/.
cp $SONIC/dockers/docker-orchagent/base_image_files/swssloglevel $TARGET/usr/bin/.
# XXX not sure which orchagent to use here

cd $X1

for pkg in $DOCKER_PKGS; do
    sudo -E -u $SUDO_USER $ONL/tools/onlpm.py --try-arches $ARCH all --build $pkg
    dpkg --root $TARGET --unpack `$ONL/tools/onlpm.py --lookup $pkg`
done

# for now syncd and swss are using init and not systemd
# see https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=805498
# UGH UGH UGH update-rc.d is not aware of systemd units

ln -s /etc/init.d/syncd $TARGET/etc/rc0.d/K01syncd
ln -s /etc/init.d/syncd $TARGET/etc/rc1.d/K01syncd
ln -s /etc/init.d/syncd $TARGET/etc/rc2.d/S90syncd
ln -s /etc/init.d/syncd $TARGET/etc/rc3.d/S90syncd
ln -s /etc/init.d/syncd $TARGET/etc/rc4.d/S90syncd
ln -s /etc/init.d/syncd $TARGET/etc/rc5.d/S90syncd
ln -s /etc/init.d/syncd $TARGET/etc/rc6.d/K01syncd

ln -s /etc/init.d/swss $TARGET/etc/rc0.d/K01swss
ln -s /etc/init.d/swss $TARGET/etc/rc1.d/K01swss
ln -s /etc/init.d/swss $TARGET/etc/rc2.d/S90swss
ln -s /etc/init.d/swss $TARGET/etc/rc3.d/S90swss
ln -s /etc/init.d/swss $TARGET/etc/rc4.d/S90swss
ln -s /etc/init.d/swss $TARGET/etc/rc5.d/S90swss
ln -s /etc/init.d/swss $TARGET/etc/rc6.d/K01swss

test -d /newroot || mkdir /newroot
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
