include $(X1)/make/config.mk
export PLATFORM_PATH=$(SONIC)/platform/broadcom/
include $(SONIC)/platform/broadcom/docker-syncd-brcm.mk

export docker := $(DOCKER_SYNCD_BASE_STEM)
export docker_image := $(docker)
export docker_image_name := $(basename $(docker))
export docker_container_name := $($(DOCKER_SYNCD_BASE)_CONTAINER_NAME)
export docker_image_run_opt := $($(DOCKER_SYNCD_BASE)_RUN_OPT)
export install_debug_image := "n"

docker_image_ctl.j2: $(SONIC)/files/build_templates/docker_image_ctl.j2
	ln -sf $^ $@

syncd.sh: docker_image_ctl.j2 edit-syncd-sh
	./edit-syncd-sh docker_image_ctl.j2 $@
	chmod +x $@

syncd.service: $(SONIC)/files/build_templates/syncd.service.j2 edit-syncd-service
	./edit-syncd-service $(SONIC)/files/build_templates/syncd.service.j2 $@
