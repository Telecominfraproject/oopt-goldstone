include $(X1)/make/config.mk

include $(SONIC)/rules/docker-database.mk

export docker := $(DOCKER_DATABASE)
export docker_image := $(docker)
export docker_image_name := $(basename $(docker))
export docker_container_name := $($(docker)_CONTAINER_NAME)
export docker_image_run_opt := $($(docker)_RUN_OPT)
export sonic_asic_platform := broadcom

all: docker-database.gz database.sh database.service

$(SONIC)/.platform:
	$(MAKE) -C $(SONIC) init
	$(MAKE) -C $(SONIC) configure PLATFORM=$(sonic_asic_platform)

docker-database.gz: $(SONIC)/.platform
	$(MAKE) -C $(SONIC) target/docker-database.gz
	ln -sf $(SONIC)/target/docker-database.gz $@

database.sh:
	j2 $(SONIC)/files/build_templates/docker_image_ctl.j2 > $@
	chmod +x $@

database.service:
	j2 $(SONIC)/files/build_templates/database.service.j2 > $@

clean:
	-rm docker-database.gz database.sh database.service
