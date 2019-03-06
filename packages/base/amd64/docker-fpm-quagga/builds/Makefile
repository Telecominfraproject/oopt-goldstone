include $(X1)/make/config.mk

include $(SONIC)/rules/docker-fpm-quagga.mk

export docker := $(DOCKER_FPM_QUAGGA)
export docker_image := $(docker)
export docker_image_name := $(basename $(docker))
export docker_container_name := $($(docker)_CONTAINER_NAME)
export docker_image_run_opt := $($(docker)_RUN_OPT)
export sonic_asic_platform := broadcom

all: docker-fpm-quagga.gz bgp.sh bgp.service

$(SONIC)/.platform:
	$(MAKE) -C $(SONIC) init
	$(MAKE) -C $(SONIC) configure PLATFORM=$(sonic_asic_platform)

docker-fpm-quagga.gz: $(SONIC)/.platform
	$(MAKE) -C $(SONIC) target/docker-fpm-quagga.gz
	ln -sf $(SONIC)/target/docker-fpm-quagga.gz $@

bgp.sh:
	j2 $(SONIC)/files/build_templates/docker_image_ctl.j2 > $@
	chmod +x $@

bgp.service:
	j2 $(SONIC)/files/build_templates/bgp.service.j2 > $@

clean:
	-rm docker-fpm-quagga.gz bgp.sh bgp.service
