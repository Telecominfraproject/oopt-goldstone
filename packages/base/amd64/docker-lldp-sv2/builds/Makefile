include $(X1)/make/config.mk

include $(SONIC)/rules/docker-lldp-sv2.mk

export docker := $(DOCKER_LLDP_SV2)
export docker_image := $(docker)
export docker_image_name := $(basename $(docker))
export docker_container_name := $($(docker)_CONTAINER_NAME)
export docker_image_run_opt := $($(docker)_RUN_OPT)
export sonic_asic_platform := broadcom

all: docker-lldp-sv2.gz lldp.sh lldp.service

$(SONIC)/.platform:
	$(MAKE) -C $(SONIC) init
	$(MAKE) -C $(SONIC) configure PLATFORM=$(sonic_asic_platform)

docker-lldp-sv2.gz: $(SONIC)/.platform
	$(MAKE) -C $(SONIC) target/docker-lldp-sv2.gz
	ln -sf $(SONIC)/target/docker-lldp-sv2.gz $@

lldp.sh:
	j2 $(SONIC)/files/build_templates/docker_image_ctl.j2 > $@
	chmod +x $@

lldp.service:
	j2 $(SONIC)/files/build_templates/lldp.service.j2 > $@

clean:
	-rm docker-lldp-sv2.gz lldp.sh lldp.service
