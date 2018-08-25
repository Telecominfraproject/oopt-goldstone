include $(X1)/make/config.mk

export sonic_asic_platform := broadcom

all: docker-syncd.gz

$(SONIC)/.platform:
	$(MAKE) -C $(SONIC) init
	$(MAKE) -C $(SONIC) configure PLATFORM=$(sonic_asic_platform)

docker-syncd.gz: $(SONIC)/.platform
	$(MAKE) -C $(SONIC) target/docker-syncd-brcm.gz
	ln -s $(SONIC)/target/docker-syncd-brcm.gz $@

clean:
	-rm docker-syncd.gz
