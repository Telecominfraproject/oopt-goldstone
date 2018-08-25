include $(X1)/make/config.mk
include $(SONIC)/platform/broadcom/docker-orchagent-brcm.mk

export docker := $(DOCKER_ORCHAGENT_BRCM)
export docker_image := $(docker)
export docker_image_name := $(basename $(docker))
export docker_container_name := $($(docker)_CONTAINER_NAME)
export docker_image_run_opt := $($(docker)_RUN_OPT)
export sonic_asic_platform := broadcom

swss.sh: docker_image_ctl.j2
	j2 $^ > $@
	chmod +x $@

swss.service:
	j2 $(SONIC)/files/build_templates/$@.j2 > $@
	sed -i -e '/opennsl/d' $@
	sed -i -e '/interfaces/d' $@
