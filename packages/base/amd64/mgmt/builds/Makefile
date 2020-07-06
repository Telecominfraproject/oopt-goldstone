IMAGE_REPO=docker.io/microsonic
IMAGES=gs-mgmt-debug
TAG=latest

mgmt-amd64.tar: FORCE
	DOCKER_BUILDKIT=1 docker build --build-arg BASE=$(IMAGE_REPO)/$(IMAGES):$(TAG) -t $(IMAGES):$(TAG) .
	docker save $(foreach image,$(IMAGES), $(image):$(TAG)) > $@

FORCE:
