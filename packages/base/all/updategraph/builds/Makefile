include $(X1)/make/config.mk

export sonic_asic_platform := broadcom

all: updategraph updategraph.service

updategraph:
	ln -s $(SONIC)/files/image_config/updategraph/updategraph $@

updategraph.service:
	j2 $(SONIC)/files/build_templates/updategraph.service.j2 > $@
