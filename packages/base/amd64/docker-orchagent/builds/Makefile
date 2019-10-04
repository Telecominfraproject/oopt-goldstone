include $(X1)/make/config.mk

export sonic_asic_platform := broadcom

ALL := docker-orchagent.gz swss.sh swss.service
.PHONY: $(ALL)

all: $(ALL)

$(SONIC)/.platform:
	$(MAKE) -C $(SONIC) init
	$(MAKE) -C $(SONIC) configure PLATFORM=$(sonic_asic_platform)

docker-orchagent.gz: $(SONIC)/.platform
	$(MAKE) -C $(SONIC) target/$@
	ln -sf $(SONIC)/target/$@ $@

swss.sh: swss.mk
	$(MAKE) -f swss.mk $@

swss.service: swss.mk
	$(MAKE) -f swss.mk $@

clean:
	-$(RM) $(ALL) *~
