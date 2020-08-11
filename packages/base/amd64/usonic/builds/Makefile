USONIC_IMAGE_REPO=docker.io/microsonic
USONIC_IMAGES=usonic-debug:201904 usonic-cli:latest

usonic-amd64.tar: Makefile
	$(foreach image,$(USONIC_IMAGES), docker pull $(USONIC_IMAGE_REPO)/$(image);)
	docker save $(foreach image,$(USONIC_IMAGES), $(USONIC_IMAGE_REPO)/$(image)) > $@
