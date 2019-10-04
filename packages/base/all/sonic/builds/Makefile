include $(X1)/make/config.mk

ALL := motd environment rc.local firsttime sonic
.PHONY : $(ALL)

all: $(ALL)
	@./mk_sonic_version.sh

export sonic_asic_platform := broadcom
export _USER := ${USER}

$(SONIC)/.platform:
	$(MAKE) -C $(SONIC) init
	$(MAKE) -C $(SONIC) configure PLATFORM=$(sonic_asic_platform)

$(SONIC)/fsroot: $(SONIC)/.platform
	$(MAKE) -C $(SONIC) target/sonic-broadcom.bin
	sudo chown -R $(_USER):$(_USER) $(SONIC)/fsroot

sonic: $(SONIC)/fsroot

motd:
	rm -rf motd
	ln -s $(SONIC)/files/image_config/environment/motd motd

environment:
	rm -rf environment
	ln -s $(SONIC)/files/image_config/environment/environment environment

rc.local: $(SONIC)/files/image_config/platform/rc.local edit-rc-local
	./edit-rc-local $(SONIC)/files/image_config/platform/rc.local $@
	chmod +x $@

firsttime:
	touch firsttime

clean:
	$(RM) $(ALL)
