USONIC_IMAGES ?= docker.io/nlpldev/usonic-cli:latest ghcr.io/microsonic/usonic-debug:201904 docker.io/bitnami/kubectl:latest
BCMD_IMAGE ?= bcmd:latest

SAI_VERSION := 20220524

.PHONY: bcmd

all: usonic-amd64.tar libsai.so

usonic-amd64.tar: Makefile bcmd
	for image in $(USONIC_IMAGES);\
	do \
	    docker pull $$image; \
	done
	docker save $(USONIC_IMAGES) $(BCMD_IMAGE) > $@

libsai.so:
	oras pull ghcr.io/oopt-goldstone/libsai:$(SAI_VERSION)

bcmd:
	cd bcmd && DOCKER_BUILDKIT=1 docker build --progress plain -t $(BCMD_IMAGE) .
