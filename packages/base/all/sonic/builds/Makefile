include $(X1)/make/config.mk

ALL := motd environment rc.local device firsttime
.PHONY : $(ALL)

all: $(ALL)
	@./mk_sonic_version.sh

motd:
	rm -rf motd
	ln -s $(SONIC)/files/image_config/environment/motd motd

environment:
	rm -rf environment
	ln -s $(SONIC)/files/image_config/environment/environment environment

rc.local: $(SONIC)/files/image_config/platform/rc.local edit-rc-local
	./edit-rc-local $(SONIC)/files/image_config/platform/rc.local $@
	chmod +x $@

device:
	rm -rf device
	mkdir device
	cp -r -H $(SONIC)/device/*/* ./device

firsttime:
	touch firsttime

clean:
	-rm -r $(ALL)
