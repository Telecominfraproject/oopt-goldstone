ifndef GOLDSTONE_BUILDER_IMAGE
    GOLDSTONE_BUILDER_IMAGE = gs-builder
endif

ifdef X1
else
X1 = $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
export X1
endif

ifdef ONL
else
ONL = $(X1)/sm/ONL
export ONL
endif

ifdef DOCKER_IMAGE
include $(ONL)/make/config.mk
else
$(warning detected host (non-docker) environment; some targets will not work)
endif

ifdef SSH_AUTH_SOCK

VOLUMES_ARGS = \
  $(shell sm/ONL/tools/scripts/gitroot) \
  /var/run/docker.sock \
  $(shell realpath $${SSH_AUTH_SOCK}) \
  # THIS LINE INTENTIONALLY LEFT BLANK

else

VOLUMES_ARGS = \
  $(shell sm/ONL/tools/scripts/gitroot) \
  /var/run/docker.sock \
  # THIS LINE INTENTIONALLY LEFT BLANK

endif

VOLUMES_OPTS = \
  --volumes $(VOLUMES_ARGS) \
  # THIS LINE INTENTIONALLY LEFT BLANK

BUILDER_OPTS = \
  --verbose \
  --image $(GOLDSTONE_BUILDER_IMAGE) \
  --hostname gsbuilder$(VERSION) \
  --workdir $(shell pwd) \
  --isolate \
  # THIS LINE INTENTIONALLY LEFT BLANK

ALL_ARCHES = arm64 amd64

# Build rule for each architecture.
define build_arch_template
$(1) :
	$(MAKE) -C builds/$(1)
endef
$(foreach a,$(ALL_ARCHES),$(eval $(call build_arch_template,$(a))))

all: builder docker


autobuild: $(ALL_ARCHES)

docker-check:
	@which docker > /dev/null || (echo "*** Docker appears to be missing. Please install docker in order to build Goldstone." && exit 1)
	@docker inspect $(GOLDSTONE_BUILDER_IMAGE) > /dev/null 2>&1 || (echo "*** Docker builder $(GOLDSTONE_BUILDER_IMAGE) doesn't exist. Please execute 'make builder' to build it." && exit 1)

# create an interative docker shell, for debugging builds
docker-debug: docker-check
	$(ONL)/docker/tools/onlbuilder $(BUILDER_OPTS) $(VOLUMES_OPTS) -c tools/debug.sh

builder:
	docker pull --platform=linux/amd64 python:3.10-buster
	docker tag python:3.10-buster python:3-buster-amd64
	cd docker/images/builder && docker build -t $(GOLDSTONE_BUILDER_IMAGE) .

docker: docker-check
	$(ONL)/docker/tools/onlbuilder $(BUILDER_OPTS) --non-interactive $(VOLUMES_OPTS) -c tools/autobuild/build.sh -b HEAD

versions:
	$(ONL)/tools/make-versions.py --import-file=$(X1)/tools/x1vi --class-name=OnlVersionImplementation --output-dir $(X1)/make/versions --force

modclean:
	rm -rf $(BUILDER_MODULE_DATABASE) $(BUILDER_MODULE_MANIFEST)

modules:
	make $(ONL)/make/config.mk

rebuild:
	$(ONLPM) --rebuild-pkg-cache

-include Make.local
