include $(X1)/make/mgmt.mk

TEMPLATE_DIR ?= $(X1)/packages/base/any/snmp/builds/templates

IMAGE_TAG ?= $(GS_MGMT_VERSION)
IMAGES ?= snmpd north-snmp
SNMP_IMAGES ?= $(addsuffix :$(IMAGE_TAG),$(addprefix $(GS_MGMT_IMAGE_REPO)/,$(IMAGES)))

.PHONY: gs-snmp-$(ARCH).tar

.DEFAULT_GOAL := all

all: gs-snmp-$(ARCH).tar manifests

gs-snmp-$(ARCH).tar:
	echo $(SNMP_IMAGES)
	for image in $(SNMP_IMAGES); do docker pull --platform=linux/$(ARCH) $$image; done
	docker save $(SNMP_IMAGES) > $@
