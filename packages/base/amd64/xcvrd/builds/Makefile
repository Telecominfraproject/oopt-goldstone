XCVRD_IMAGE ?= xcvrd:latest
REDIS_IMAGE ?= redis:latest
sonic_buildimage_xcvrd_dir ?= $(X1)/sm/sonic-buildimage-xcvrd

all: xcvrd.tar

xcvrd.tar:
	docker buildx build --build-context sonic_buildimage_xcvrd_dir=$(sonic_buildimage_xcvrd_dir) \
				-t $(XCVRD_IMAGE) .
	docker pull $(REDIS_IMAGE)
	docker save $(XCVRD_IMAGE) $(REDIS_IMAGE) -o $@
