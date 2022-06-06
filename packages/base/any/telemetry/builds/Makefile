include $(X1)/make/mgmt.mk

TEMPLATE_DIR ?= $(X1)/packages/base/any/telemetry/builds/templates

IMAGE_TAG ?= $(GS_MGMT_VERSION)
IMAGES ?= system-telemetry
SYSTEM_TELEMETRY_IMAGES ?= $(addsuffix :$(IMAGE_TAG),$(addprefix $(GS_MGMT_IMAGE_REPO)/,$(IMAGES)))

.PHONY: gs-system-telemetry-$(ARCH).tar

.DEFAULT_GOAL := all

all: gs-system-telemetry-$(ARCH).tar manifests

gs-system-telemetry-$(ARCH).tar:
	for image in $(SYSTEM_TELEMETRY_IMAGES); do docker pull --platform=linux/$(ARCH) $$image; done
	docker save $(SYSTEM_TELEMETRY_IMAGES) > $@
