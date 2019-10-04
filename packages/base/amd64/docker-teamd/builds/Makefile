include $(X1)/make/config.mk

include $(SONIC)/rules/docker-teamd.mk

export docker := $(DOCKER_TEAMD)
export docker_image := $(docker)
export docker_image_name := $(basename $(docker))
export docker_container_name := $($(docker)_CONTAINER_NAME)
export docker_image_run_opt := $($(docker)_RUN_OPT)
export sonic_asic_platform := broadcom
export install_debug_image := "n"

all: docker-teamd.gz teamd.sh teamd.service

$(SONIC)/.platform:
	$(MAKE) -C $(SONIC) init
	$(MAKE) -C $(SONIC) configure PLATFORM=$(sonic_asic_platform)

docker-teamd.gz: $(SONIC)/.platform
	$(MAKE) -C $(SONIC) target/docker-teamd.gz
	ln -sf $(SONIC)/target/docker-teamd.gz $@

teamd.sh:
	j2 $(SONIC)/files/build_templates/docker_image_ctl.j2 > $@
	chmod +x $@

teamd.service:
	j2 $(SONIC)/files/build_templates/teamd.service.j2 > $@

clean:
	-rm docker-teamd.gz teamd.sh teamd.service
