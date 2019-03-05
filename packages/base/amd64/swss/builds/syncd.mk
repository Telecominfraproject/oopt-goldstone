include $(X1)/make/config.mk
include $(SONIC)/platform/broadcom/docker-syncd-brcm.mk

export docker := $(DOCKER_SYNCD_BRCM)
export docker_image := $(docker)
export docker_image_name := $(basename $(docker))
export docker_container_name := $($(docker)_CONTAINER_NAME)
export docker_image_run_opt := $($(docker)_RUN_OPT)
export sonic_asic_platform := broadcom

syncd.sh: docker_image_ctl.j2 syncd.sh.pre
	cp /dev/null $@
	cat syncd.sh.pre >> $@
	j2 docker_image_ctl.j2 >> $@
	chmod +x $@

syncd.service: $(SONIC)/files/build_templates/syncd.service.j2 edit-syncd-service
	./edit-syncd-service $(SONIC)/files/build_templates/syncd.service.j2 $@
