# -*- Makefile -*-
THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

include $(ONL)/make/config.arm64.mk

K_TARGET_DIR := $(THIS_DIR)

KERNEL_DIR ?= $(X1)/sm/linux-gft

kernel:
	$(MAKE) -C $(X1)/packages/base/any/kernels/5.4-lts/configs/arm64-all K_TARGET_DIR=$(K_TARGET_DIR) K_SOURCE_DIR=$(KERNEL_DIR) K_MBUILD_DIR=$(THIS_DIR)/linux-5.4-mbuild K_DTBS_DIR=$(THIS_DIR)/linux-5.4-dtbs $(ONL_MAKE_PARALLEL)
	ARCH=$(ARCH) $(ONL)/tools/scripts/kmodbuild.sh $(KERNEL_DIR) "$(wildcard $(ONL)/packages/base/any/kernels/modules/optoe.c)" onl/onl/common

clean:
	rm -rf linux-5.4* kernel-* lib
