#!/bin/sh

set -eu

TARGET=`realpath $1`

PYTHON_PKGS=`cat $X1/builds/any/rootfs/python-packages.yml | python -c 'import sys; print " ".join( l.strip()[2:] for l in sys.stdin )'`

cd $X1

chroot $TARGET pip install natsort

for pkg in $PYTHON_PKGS; do
    WHEEL=target/python-wheels/$pkg
    sudo -E -u $SUDO_USER make -C $SONIC $WHEEL
    cp $SONIC/$WHEEL $TARGET/tmp/
    chroot $TARGET pip install /tmp/$pkg
done
