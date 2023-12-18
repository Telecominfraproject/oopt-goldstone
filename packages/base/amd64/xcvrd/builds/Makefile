XCVRD_IMAGE ?= xcvrd:latest
REDIS_IMAGE ?= redis:latest
sonic_buildimage_xcvrd_dir ?= $(X1)/sm/sonic-buildimage-xcvrd
sm_onl_dir ?= $(X1)/sm/ONL
git_modules_sm_onl_dir ?= $(X1)/.git/modules/sm/ONL

all: xcvrd.tar

xcvrd.tar:
	docker buildx build --build-context sonic_buildimage_xcvrd_dir=$(sonic_buildimage_xcvrd_dir) \
				--build-context sm_onl_dir=$(sm_onl_dir) \
				--build-context git_modules_sm_onl_dir=$(git_modules_sm_onl_dir) \
				-t $(XCVRD_IMAGE) .
	docker pull $(REDIS_IMAGE)
	docker save $(XCVRD_IMAGE) $(REDIS_IMAGE) -o $@
