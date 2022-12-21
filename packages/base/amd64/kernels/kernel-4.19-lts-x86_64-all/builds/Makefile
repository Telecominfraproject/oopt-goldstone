KERNEL_BUILD_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
KERNEL_ARCH := x86_64
KERNEL_LTS_VERSION := 4.19
KERNEL_CONFIG := x86_64-all

include $(ONL)/make/config.mk

export K_TARGET_DIR := $(KERNEL_BUILD_DIR)/$(ONL_DEBIAN_SUITE)

kernel: setup
	$(MAKE) -C $(X1)/packages/base/any/kernels/$(KERNEL_LTS_VERSION)-lts/configs/$(KERNEL_CONFIG) $(ONL_MAKE_PARALLEL)
	cd $(K_TARGET_DIR) && ARCH=$(KERNEL_ARCH) $(ONL)/tools/scripts/kmodbuild.sh linux-*-mbuild "$(wildcard $(ONL)/packages/base/any/kernels/modules/*)" onl/onl/common

clean:
	rm -rf $(K_TARGET_DIR)

setup::
	dpkg -l libelf-dev > /dev/null 2>&1 || sudo apt-get install libelf-dev
