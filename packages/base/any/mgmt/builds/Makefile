include $(X1)/make/mgmt.mk

TEMPLATE_DIR ?= templates

GS_MGMT_VERSION ?= latest

IMAGE_TAG ?= $(GS_MGMT_VERSION)
MGMT_BUILDER ?= $(GS_MGMT_IMAGE_REPO)/host-packages:$(IMAGE_TAG)
YANG_CONTAINER ?= $(GS_MGMT_IMAGE_REPO)/south-onlp:$(IMAGE_TAG)
IMAGES ?= south-onlp south-tai north-notif
KUBECTL_IMAGE ?= lachlanevenson/k8s-kubectl:latest # multi-arch image

ifeq ($(ARCH), arm64)
    IMAGES += south-gearbox south-dpll
else ifeq ($(ARCH), amd64)
    IMAGES += south-sonic
endif

MGMT_IMAGES ?= $(addsuffix :$(IMAGE_TAG),$(addprefix $(GS_MGMT_IMAGE_REPO)/,$(IMAGES)))

.PHONY: wheels debs yang

.DEFAULT_GOAL := all

all: wheels debs yang mgmt-$(ARCH).tar k8s-$(ARCH).tar gs-yang.py manifests

mgmt-$(ARCH).tar: FORCE
	for image in $(MGMT_IMAGES); do docker pull --platform=linux/$(ARCH) $$image; done
	docker save $(MGMT_IMAGES) > $@

k8s-$(ARCH).tar: FORCE
	docker pull --platform=linux/$(ARCH) $(KUBECTL_IMAGE)
	docker save $(KUBECTL_IMAGE) > $@

host-image:
	docker pull --platform=linux/$(ARCH) $(MGMT_BUILDER)

yang-image:
	docker pull --platform=linux/$(ARCH) $(YANG_CONTAINER)

debs: host-image
	$(RM) -r debs && mkdir debs
	docker run -v `pwd`/debs:/data -w /data $(MGMT_BUILDER) sh -c "cp /usr/share/debs/libyang/libyang1_*.deb /data/libyang1_$(ARCH).deb"
	docker run -v `pwd`/debs:/data -w /data $(MGMT_BUILDER) sh -c "cp /usr/share/debs/sysrepo/sysrepo_*.deb /data/sysrepo_$(ARCH).deb"
	docker run -v `pwd`/debs:/data -w /data $(MGMT_BUILDER) sh -c "cp /usr/share/debs/tacacs/libtac2_*.deb /data/libtac2_$(ARCH).deb"
	docker run -v `pwd`/debs:/data -w /data $(MGMT_BUILDER) sh -c "cp /usr/share/debs/tacacs/libtac2-bin_*.deb /data/libtac2-bin_$(ARCH).deb"
	docker run -v `pwd`/debs:/data -w /data $(MGMT_BUILDER) sh -c "cp /usr/share/debs/tacacs/libpam-tacplus_*.deb /data/libpam-tacplus_$(ARCH).deb"
	docker run -v `pwd`/debs:/data -w /data $(MGMT_BUILDER) sh -c "cp /usr/share/debs/tacacs/libnss-tacplus_*.deb /data/libnss-tacplus_$(ARCH).deb"

wheels: host-image
	$(RM) -r wheels && mkdir wheels && cd wheels && mkdir libyang sysrepo lib cli system
	docker run -v `pwd`/wheels:/data -w /data $(MGMT_BUILDER) sh -c "cp /usr/share/wheels/libyang/*.whl  /data/libyang"
	docker run -v `pwd`/wheels:/data -w /data $(MGMT_BUILDER) sh -c "cp /usr/share/wheels/sysrepo/*.whl  /data/sysrepo"
	docker run -v `pwd`/wheels:/data -w /data $(MGMT_BUILDER) sh -c "cp /usr/share/wheels/lib/*.whl  /data/lib"
	docker run -v `pwd`/wheels:/data -w /data $(MGMT_BUILDER) sh -c "cp /usr/share/wheels/cli/*.whl  /data/cli"
	docker run -v `pwd`/wheels:/data -w /data $(MGMT_BUILDER) sh -c "cp /usr/share/wheels/system/*.whl  /data/system"

yang: yang-image
	$(RM) -r yang && mkdir yang
	docker run --rm -u `id -u`:`id -g` -v `pwd`:/data -w /data --rm $(YANG_CONTAINER) sh -c "cp -r /var/lib/goldstone/yang ."

gs-yang.py: yang-image
	$(RM) -r $@
	docker run --rm -u `id -u`:`id -g` -v `pwd`:/data -w /data --rm $(YANG_CONTAINER) sh -c "cp /usr/local/bin/gs-yang.py ."

FORCE:
