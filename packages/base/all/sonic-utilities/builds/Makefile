include $(X1)/make/config.mk

include $(SONIC)/rules/python-click.mk
include $(SONIC)/rules/sonic-utilities.mk

all: python-sonic-utilities_all.deb python-click_all.deb

python-sonic-utilities_all.deb:
	$(MAKE) -C $(SONIC) target/debs/$(SONIC_UTILS)
	ln -sf $(SONIC)/target/debs/$(SONIC_UTILS) $@

python-click_all.deb:
	$(MAKE) -C $(SONIC) target/debs/$(PYTHON_CLICK)
	ln -sf $(SONIC)/target/debs/$(PYTHON_CLICK) $@
