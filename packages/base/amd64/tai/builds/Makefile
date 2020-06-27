TAI_IMAGE_REPO=docker.io/wataru
TAI_IMAGES=tai
TAI_TAG=latest

tai-amd64.tar: FORCE
	DOCKER_BUILDKIT=1 docker build --build-arg BASE=$(TAI_IMAGE_REPO)/$(TAI_IMAGES):$(TAI_TAG) -t $(TAI_IMAGES):$(TAI_TAG) .
	docker save $(foreach image,$(TAI_IMAGES), $(image):$(TAI_TAG)) > $@

FORCE:
