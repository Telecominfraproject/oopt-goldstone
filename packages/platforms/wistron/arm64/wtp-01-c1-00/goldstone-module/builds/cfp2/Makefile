# -*- Makefile -*-
############################################################
export OUTPUT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
export KERNEL := goldstone-kernel-5.4-lts-arm64-all:arm64

include $(X1)/make/config.arm64.mk

MODULE_DIR ?= $(OUTPUT_DIR)

KBUILD ?= $(shell $(ONL)/tools/onlpm.py --find-dir $(KERNEL) mbuilds)

ifndef INSTALL_MOD_PATH
INSTALL_MOD_PATH := $(OUTPUT_DIR)
endif

all:
	$(RM) -r $(OUTPUT_DIR)/lib
	test -d $(KBUILD)
	$(MAKE) -C $(KBUILD) M=$(MODULE_DIR) modules
	$(MAKE) -C $(KBUILD) M=$(MODULE_DIR) INSTALL_MOD_PATH=$(INSTALL_MOD_PATH) INSTALL_MOD_DIR=$(INSTALL_MOD_DIR) modules_install
	sudo $(RM) $(shell find $(OUTPUT_DIR)/lib -name \*modules\* -type f)

clean:
	$(MAKE) -C $(KBUILD) M=$(MODULE_DIR) clean
