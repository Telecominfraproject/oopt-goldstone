include $(X1)/make/config.mk

export sonic_asic_platform := broadcom

ALL := docker-syncd.gz syncd.sh syncd.service
.PHONY : $(ALL)

all: $(ALL)

$(SONIC)/.platform:
	$(MAKE) -C $(SONIC) init
	$(MAKE) -C $(SONIC) configure PLATFORM=$(sonic_asic_platform)

docker-syncd.gz: $(SONIC)/.platform
	$(MAKE) -C $(SONIC) target/docker-syncd-brcm.gz
	ln -sf $(SONIC)/target/docker-syncd-brcm.gz $@

syncd.sh: syncd.mk
	$(MAKE) -f syncd.mk $@

syncd.service: syncd.mk
	$(MAKE) -f syncd.mk $@

clean:
	-$(RM) $(ALL) *~
