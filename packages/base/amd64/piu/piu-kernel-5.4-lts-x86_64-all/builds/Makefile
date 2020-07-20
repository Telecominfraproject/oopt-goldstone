# -*- Makefile -*-
############################################################
export OUTPUT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
export KERNEL := onl-kernel-5.4-lts-x86-64-all:amd64
export ARCH := x86_64

all:
	$(MAKE) -C $(X1)/packages/base/any/piu/kbuilds
