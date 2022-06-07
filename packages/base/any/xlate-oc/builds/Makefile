include $(X1)/make/mgmt.mk

TEMPLATE_DIR ?= $(X1)/packages/base/any/xlate-oc/builds/templates

IMAGE_TAG ?= $(GS_MGMT_VERSION)
IMAGES ?= xlate-oc
XLATE_OC_IMAGES ?= $(addsuffix :$(IMAGE_TAG),$(addprefix $(GS_MGMT_IMAGE_REPO)/,$(IMAGES)))

.PHONY: gs-xlate-oc-$(ARCH).tar

.DEFAULT_GOAL := all

all: gs-xlate-oc-$(ARCH).tar manifests

gs-xlate-oc-$(ARCH).tar:
	for image in $(XLATE_OC_IMAGES); do docker pull --platform=linux/$(ARCH) $$image; done
	docker save $(XLATE_OC_IMAGES) > $@
