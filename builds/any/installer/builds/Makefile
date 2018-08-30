ifndef ARCH
$(error $$ARCH not set)
endif

ifndef BOOTMODE
$(error $$BOOTMODE not set)
endif

# Hardcoded to match ONL File naming conventions.
include $(X1)/make/versions/version-x1.mk
INSTALLER_NAME=$(FNAME_PRODUCT_VERSION)_ONL-OS_$(FNAME_BUILD_ID)_$(UARCH)_$(BOOTMODE)_INSTALLER

MKINSTALLER_OPTS :=   --arch $(ARCH)   --boot-config boot-config   --swi x1-swi:$(ARCH)


ifeq ($(ARCH_BOOT),uboot)
    MKINSTALLER_OPTS += --fit x1-loader-fit:$(ARCH) x1-loader-fit.itb
else ifeq ($(ARCH_BOOT),grub)
    MKINSTALLER_OPTS += --initrd x1-loader-initrd:$(ARCH) x1-loader-initrd-$(ARCH).cpio.gz
else
    $(error ARCH_BOOT=$(ARCH_BOOT) not recognized.)
endif

__installer:
	-rm -f *md5sum *_INSTALLER
	$(ONL)/tools/mkinstaller.py $(MKINSTALLER_OPTS) --out $(INSTALLER_NAME)
	md5sum "$(INSTALLER_NAME)" | awk '{ print $$1 }' > "$(INSTALLER_NAME).md5sum"
