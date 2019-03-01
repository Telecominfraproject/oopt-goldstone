# -*- Makefile -*-
############################################################
export OUTPUT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
export KERNEL := onl-kernel-4.14-lts-x86-64-all:amd64
export ARCH := x86_64
export SDK := $(X1)/sm/sonic-buildimage/platform/broadcom/saibcm-modules
export ccflags-extra = -Dpci_enable_msix=pci_enable_msix_exact -include linux/uaccess.h -include linux/sched/signal.h

all:
	$(MAKE) -C $(X1)/packages/base/any/sai/bcm/saibcm-kernel/kbuilds
