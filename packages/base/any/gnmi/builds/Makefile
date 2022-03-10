include $(X1)/make/mgmt.mk

TEMPLATE_DIR ?= $(X1)/packages/base/any/gnmi/builds/templates

IMAGE_TAG ?= $(GS_MGMT_VERSION)
IMAGES ?= north-gnmi
NORTH_GNMI_IMAGES ?= $(addsuffix :$(IMAGE_TAG),$(addprefix $(GS_MGMT_IMAGE_REPO)/,$(IMAGES)))

.PHONY: gs-north-gnmi-$(ARCH).tar

.DEFAULT_GOAL := all

all: gs-north-gnmi-$(ARCH).tar manifests

gs-north-gnmi-$(ARCH).tar:
	for image in $(NORTH_GNMI_IMAGES); do docker pull --platform=linux/$(ARCH) $$image; done
	docker save $(NORTH_GNMI_IMAGES) > $@
