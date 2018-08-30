include $(X1)/make/config.mk

ALL := syncd.sh swss.sh swss.service

all: $(ALL)

docker_image_ctl.j2:
	cp $(SONIC)/files/build_templates/$@ .
	sed -i -e '90i\ \ \ \ \ \ \ \ --tmpfs /var/log/supervisor \\' $@
	sed -i -e '90i\ \ \ \ \ \ \ \ --tmpfs /var \\' $@

swss.sh: docker_image_ctl.j2
	$(MAKE) -f swss.mk $@

swss.service:
	$(MAKE) -f swss.mk $@

syncd.sh: docker_image_ctl.j2
	$(MAKE) -f syncd.mk $@

clean:
	-rm $(ALL)
