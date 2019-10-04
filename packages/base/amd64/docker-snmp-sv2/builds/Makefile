
include $(X1)/make/config.mk

include $(SONIC)/rules/docker-snmp-sv2.mk

export docker := $(DOCKER_SNMP_SV2)
export docker_image := $(docker)
export docker_image_name := $(basename $(docker))
export docker_container_name := $($(docker)_CONTAINER_NAME)
export docker_image_run_opt := $($(docker)_RUN_OPT)
export sonic_asic_platform := broadcom
export install_debug_image := "n"

all: docker-snmp-sv2.gz snmp.sh snmp.service

.PHONY: $(all)

$(SONIC)/.platform:
	$(MAKE) -C $(SONIC) init
	$(MAKE) -C $(SONIC) configure PLATFORM=$(sonic_asic_platform)

docker-snmp-sv2.gz: $(SONIC)/.platform
	$(MAKE) -C $(SONIC) target/docker-snmp-sv2.gz
	ln -sf $(SONIC)/target/docker-snmp-sv2.gz $@

snmp.sh:
	j2 $(SONIC)/files/build_templates/docker_image_ctl.j2 > $@
	chmod +x $@

snmp.service:
	j2 $(SONIC)/files/build_templates/snmp.service.j2 > $@

clean:
	-rm docker-snmp-sv2.gz snmp.sh database.service
