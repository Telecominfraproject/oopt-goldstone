include $(X1)/make/config.mk

include $(SONIC)/rules/docker-router-advertiser.mk

export docker := $(DOCKER_ROUTER_ADVERTISER)
export docker_image := $(docker)
export docker_image_name := $(basename $(docker))
export docker_container_name := $($(docker)_CONTAINER_NAME)
export docker_image_run_opt := $($(docker)_RUN_OPT)
export sonic_asic_platform := broadcom

all: docker-router-advertiser.gz radv.sh radv.service

$(SONIC)/.platform:
	$(MAKE) -C $(SONIC) init
	$(MAKE) -C $(SONIC) configure PLATFORM=$(sonic_asic_platform)

docker-router-advertiser.gz: $(SONIC)/.platform
	$(MAKE) -C $(SONIC) target/docker-router-advertiser.gz
	ln -sf $(SONIC)/target/docker-router-advertiser.gz $@

radv.sh:
	j2 $(SONIC)/files/build_templates/docker_image_ctl.j2 > $@
	chmod +x $@

radv.service:
	j2 $(SONIC)/files/build_templates/radv.service.j2 > $@

clean:
	-rm docker-router-advertiser.gz radv.sh radv.service
