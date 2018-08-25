all: autobuild

autobuild:
	$(MAKE) -C builds/amd64

docker_check:
	@which docker > /dev/null || (echo "*** Docker appears to be missing. Please install docker-io in order to build X1." && exit 1)

# create an interative docker shell, for debugging builds
docker-debug: docker_check
	@sm/ONL/docker/tools/onlbuilder -8 --isolate `pwd` /var/run/docker.sock --hostname x1builder$(VERSION) -c bash

docker: docker_check
	@sm/ONL/docker/tools/onlbuilder -8 --isolate `pwd` /var/run/docker.sock --hostname x1builder$(VERSION) -c tools/autobuild/build.sh -b HEAD

sonic:
	$(MAKE) -C sm/sonic-buildimage init
	$(MAKE) -C sm/sonic-buildimage configure PLATFORM=broadcom
	$(MAKE) -C sm/sonic-buildimage
