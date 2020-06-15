include $(X1)/make/config.amd64.mk

# All amd64 kernels
KERNELS := $(shell $(ONLPM) --find-file onl-kernel-5.4-lts-x86-64-all:amd64 kernel-5.4-lts-x86_64-all)

# Loader initrd
INITRD := $(shell $(ONLPM) --find-file x1-loader-initrd:amd64 x1-loader-initrd-amd64.cpio.gz)
MANIFEST := $(shell $(ONLPM) --find-file x1-loader-initrd:amd64 manifest.json)

all:
	mkdir -p files
	cp $(KERNELS) files
	cp $(INITRD) files
	cp $(MANIFEST) files

