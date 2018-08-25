include $(X1)/make/config.mk

export sonic_asic_platform := broadcom

all: docker-orchagent.gz

$(SONIC)/.platform:
	$(MAKE) -C $(SONIC) init
	$(MAKE) -C $(SONIC) configure PLATFORM=$(sonic_asic_platform)

docker-orchagent.gz: $(SONIC)/.platform
	$(MAKE) -C $(SONIC) target/docker-orchagent-brcm.gz
	ln -s $(SONIC)/target/docker-orchagent-brcm.gz $@

clean:
	-rm docker-orchagent.gz
