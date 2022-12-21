THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
include $(ONL)/make/config.mk

export ARCH := x86_64
ifndef K_TARGET_DIR
K_TARGET_DIR := $(THIS_DIR)
endif

include ../../kconfig.mk
K_CONFIG := x86_64-all.config
K_BUILD_TARGET := bzImage
K_COPY_SRC := arch/x86/boot/bzImage
ifndef K_COPY_DST
K_COPY_DST := kernel-4.19-lts-x86_64-all
endif

include $(ONL)/make/kbuild.mk
