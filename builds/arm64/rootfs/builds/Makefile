include $(X1)/make/config.arm64.mk

ifndef PLATFORM_LIST
    export PLATFORM_LIST=$(shell onlpm --list-platforms --arch arm64 --csv )
endif

RFS_CONFIG := $(X1)/builds/any/rootfs/rfs.yml

ifndef ARCH
$(error $$ARCH must be specified)
endif

ifndef RFS_CONFIG
$(error $$RFS_CONFIG must be set to the RFS yaml configuration file)
endif

ifndef RFS_WORKDIR
RFS_WORKDIR := $(ONL_DEBIAN_SUITE)
endif

ifndef RFS_DIR
RFS_DIR := $(RFS_WORKDIR)/rootfs-$(ARCH).d
endif

ifndef RFS_SQUASH
RFS_SQUASH := $(RFS_WORKDIR)/rootfs-$(ARCH).sqsh
endif

RFS_COMMAND := $(ONL)/tools/onlrfs.py --config $(RFS_CONFIG) --arch $(ARCH) --dir $(RFS_DIR)

ifdef RFS_CPIO
RFS_COMMAND += --cpio $(RFS_CPIO)
endif

ifdef RFS_SQUASH
RFS_COMMAND += --squash $(RFS_SQUASH)
endif

ifndef RFS_MANIFEST
RFS_MANIFEST := etc/goldstone/rootfs/manifest.json
endif

LOCAL_MANIFEST := $(RFS_WORKDIR)/manifest.json

RFS: clean
	$(ONL_V_at) $(RFS_COMMAND)
	$(ONL_V_at) [ -f $(RFS_DIR)/$(RFS_MANIFEST) ] && sudo cp $(RFS_DIR)/$(RFS_MANIFEST) $(LOCAL_MANIFEST)

clean:
	$(ONL_V_at) sudo rm -rf $(RFS_WORKDIR)

show-packages:
	$(ONL_V_at) $(RFS_COMMAND) --show-packages
