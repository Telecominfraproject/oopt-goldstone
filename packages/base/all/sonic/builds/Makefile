include $(X1)/make/config.mk

ALL := motd environment rc.local device firsttime

all: $(ALL)
	@./mk_sonic_version.sh

motd:
	ln -s $(SONIC)/files/image_config/environment/motd motd

environment:
	ln -s $(SONIC)/files/image_config/environment/environment environment

rc.local: $(SONIC)/files/image_config/platform/rc.local edit-rc-local
	./edit-rc-local $(SONIC)/files/image_config/platform/rc.local $@

device:
	mkdir device
	cp -r -H $(SONIC)/device/*/* ./device

firsttime:
	touch firsttime

clean:
	-rm -r $(ALL)
