#!/bin/bash
############################################################
set -e

AUTOBUILD_SCRIPT="$(realpath ${BASH_SOURCE[0]})"
X1="$(realpath $(dirname $AUTOBUILD_SCRIPT)/../../)"


# Default build branch
BUILD_BRANCH=master

while getopts "c:bvV:r" opt; do
    case $opt in
        c)
            cd $ONL && git submodule update --init --recursive packages/platforms-closed
            ;;
        b)
            BUILD_BRANCH=$OPTARG
            ;;
        v)
            set -x
            ;;
        V)
            export VERBOSE=1
            ;;
        r)
            export BUILDROOTMIRROR=$OPTARG
            ;;
        *)
            ;;
    esac
done

# The expectation is that we will already be on the required branch.
# This is to normalize environments where the checkout might instead
# be in a detached head (like jenkins)
echo "Switching to branch $BUILD_BRANCH..."
cd $X1 && git checkout $BUILD_BRANCH

. $X1/setup.env


#
# Restart under correct builder environment.
#
ONLB_OPTIONS=-9
if [ -z "$DOCKER_IMAGE" ]; then
    # Execute ourselves under the builder
    ONLB=$ONL/docker/tools/onlbuilder
    if [ -x $ONLB ]; then
        $ONLB $ONLB_OPTIONS --volumes $ONL --non-interactive -c $AUTOBUILD_SCRIPT $@
        exit $?
    else
        echo "Not running in a docker workspace and the onlbuilder script is not available."
        exit 1
    fi
fi

echo "Now running under $DOCKER_IMAGE..."


#
# Full build
#
cd $X1
. setup.env

if ! make autobuild; then
    echo Build Failed.
    exit 1
fi

make -C $X1/REPO build-clean

# Remove all installer/rootfs/swi packages from the repo. These do not need to be kept and take significant
# amounts of time to transfer.
find $X1/REPO -name "*-installer_0.*" -delete
find $X1/REPO -name "*-rootfs_0.*" -delete
find $X1/REPO -name "*-swi_0*" -delete

echo Build Succeeded.
