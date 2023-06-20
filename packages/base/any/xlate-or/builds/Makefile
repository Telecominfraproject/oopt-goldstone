include $(X1)/make/mgmt.mk

TEMPLATE_DIR ?= $(X1)/packages/base/any/xlate-or/builds/templates

IMAGE_TAG ?= $(GS_MGMT_VERSION)
IMAGES ?= xlate-or
XLATE_OR_IMAGES ?= $(addsuffix :$(IMAGE_TAG),$(addprefix $(GS_MGMT_IMAGE_REPO)/,$(IMAGES)))

.PHONY: gs-xlate-or-$(ARCH).tar

.DEFAULT_GOAL := all

all: gs-xlate-or-$(ARCH).tar manifests

gs-xlate-or-$(ARCH).tar:
	for image in $(XLATE_OR_IMAGES); do docker pull --platform=linux/$(ARCH) $$image; done
	docker save $(XLATE_OR_IMAGES) > $@
