include $(X1)/make/mgmt.mk

TEMPLATE_DIR ?= $(X1)/packages/base/any/netopeer2/builds/templates

IMAGE_TAG ?= $(GS_MGMT_VERSION)
NETOPEER2_IMAGE ?= $(GS_MGMT_IMAGE_REPO)/north-netconf:$(IMAGE_TAG)

.PHONY: yang image scripts

.DEFAULT_GOAL := all

all: netopeer2-$(ARCH).tar yang scripts manifests

image:
	docker pull --platform=linux/$(ARCH) $(NETOPEER2_IMAGE)

netopeer2-$(ARCH).tar: image
	docker save $(NETOPEER2_IMAGE) > $@

yang: image
	echo `pwd`
	$(RM) -r $@ && mkdir $@
	docker run -v `pwd`/yang:/yang -w /yang --rm $(NETOPEER2_IMAGE) sh -c "cp /usr/local/share/yang/modules/netopeer2/*.yang /yang"

scripts: image
	$(RM) -r $@ && mkdir $@
	docker run -v `pwd`/scripts:/scripts -w /scripts --rm $(NETOPEER2_IMAGE) sh -c "cp /usr/local/share/netopeer2/scripts/*.sh /scripts"
