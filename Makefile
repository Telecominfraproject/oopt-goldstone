ifndef X1_BUILDER_IMAGE
X1_BUILDER_IMAGE := opennetworklinux/x1-builder8
endif

all: autobuild

autobuild:
	$(MAKE) -C builds/amd64

docker_check:
	@which docker > /dev/null || (echo "*** Docker appears to be missing. Please install docker-io in order to build X1." && exit 1)

# create an interative docker shell, for debugging builds
docker-debug: docker_check
	@sm/ONL/docker/tools/onlbuilder --image $(X1_BUILDER_IMAGE) --isolate --hostname x1builder$(VERSION) -c bash

docker: docker_check
	@sm/ONL/docker/tools/onlbuilder --image $(X1_BUILDER_IMAGE) --isolate --hostname x1builder$(VERSION) -c tools/autobuild/build.sh -b HEAD

build: docker_check
	$(MAKE) -C docker/images/builder/builder8/1.0 $@
