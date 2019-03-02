include $(X1)/make/config.mk

ALL := syncd.sh syncd.service swss.sh swss.service

all: $(ALL)

docker_image_ctl.j2: $(SONIC)/files/build_templates/docker_image_ctl.j2 Makefile
	cp $(SONIC)/files/build_templates/docker_image_ctl.j2 .
	sed -i -e '141i\ \ \ \ \ \ \ \ --tmpfs /var/log/supervisor \\' $@
	sed -i -e '141i\ \ \ \ \ \ \ \ --tmpfs /var \\' $@

swss.sh: docker_image_ctl.j2
	$(MAKE) -f swss.mk $@

swss.service: swss.mk
	$(MAKE) -f swss.mk $@

syncd.sh: docker_image_ctl.j2
	$(MAKE) -f syncd.mk $@

syncd.service: swss.mk
	$(MAKE) -f swss.mk $@

clean:
	-rm $(ALL)
