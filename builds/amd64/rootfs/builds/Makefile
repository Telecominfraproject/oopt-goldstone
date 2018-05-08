include $(X1)/make/config.amd64.mk

export PLATFORM_LIST=$(shell onlpm --list-platforms --arch amd64 --csv )

RFS_CONFIG := $(X1)/builds/any/rootfs/rfs.yml
RFS_DIR := rootfs-amd64.d
RFS_CPIO := rootfs-amd64.cpio.gz
RFS_SQUASH := rootfs-amd64.sqsh
RFS_MANIFEST := /etc/x1/rootfs/manifest.json

include $(ONL)/make/rfs.mk
