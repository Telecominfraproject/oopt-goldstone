include $(X1)/make/config.mk

ALL := motd environment rc.local device firsttime

all: $(ALL)
	@./mk_sonic_version.sh

motd:
	ln -s $(SONIC)/files/image_config/environment/motd motd

environment:
	ln -s $(SONIC)/files/image_config/environment/environment environment

rc.local:
	cp $(SONIC)/files/image_config/platform/$@ $@
	sed -i -e "13i ip li set up dev eth0" $@
	sed -i -e "13i ip li show dev ma1 &> /dev/null && ip li set name eth0 dev ma1" $@
	sed -i -e "13i ip li show dev ma1 &> /dev/null && ip li set down dev ma1" $@
	sed -i -e "13i # temporal hack for X1" $@

device:
	mkdir device
	cp -r -H $(SONIC)/device/*/* ./device

firsttime:
	touch firsttime

clean:
	-rm -r $(ALL)
