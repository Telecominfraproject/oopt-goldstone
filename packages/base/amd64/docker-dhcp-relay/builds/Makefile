include $(X1)/make/config.mk

include $(SONIC)/rules/docker-dhcp-relay.mk

export docker := $(DOCKER_DHCP_RELAY)
export docker_image := $(docker)
export docker_image_name := $(basename $(docker))
export docker_container_name := $($(docker)_CONTAINER_NAME)
export docker_image_run_opt := $($(docker)_RUN_OPT)
export sonic_asic_platform := broadcom
export install_debug_image := "n"

all: docker-dhcp-relay.gz dhcp_relay.sh dhcp_relay.service

$(SONIC)/.platform:
	$(MAKE) -C $(SONIC) init
	$(MAKE) -C $(SONIC) configure PLATFORM=$(sonic_asic_platform)

docker-dhcp-relay.gz: $(SONIC)/.platform
	$(MAKE) -C $(SONIC) target/docker-dhcp-relay.gz
	ln -sf $(SONIC)/target/docker-dhcp-relay.gz $@

dhcp_relay.sh:
	j2 $(SONIC)/files/build_templates/docker_image_ctl.j2 > $@
	chmod +x $@

dhcp_relay.service:
	j2 $(SONIC)/files/build_templates/dhcp_relay.service.j2 > $@

clean:
	-rm docker-dhcp-relay.gz dhcp_relay.sh dhcp_relay.service
