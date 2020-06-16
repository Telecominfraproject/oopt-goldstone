USONIC_IMAGE_REPO=docker.io/wataru
USONIC_IMAGES=usonic-debug usonic-cli
USONIC_TAG=201811

usonic-amd64.tar:
	$(foreach image,$(USONIC_IMAGES), docker pull $(USONIC_IMAGE_REPO)/$(image):$(USONIC_TAG); docker tag $(USONIC_IMAGE_REPO)/$(image):$(USONIC_TAG) $(image):$(USONIC_TAG);)
	docker save $(foreach image,$(USONIC_IMAGES), $(image):$(USONIC_TAG)) > $@
