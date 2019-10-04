include $(X1)/make/config.mk

include $(SONIC)/rules/python-click.mk
include $(SONIC)/rules/sonic-utilities.mk

all: python-sonic-utilities_all.deb python-click_all.deb

python-sonic-utilities_all.deb:
	@: ;\
	set -x; set -e ;\
	if test -d $(SONIC)/target/python-debs; then \
	  $(MAKE) -C $(SONIC) target/python-debs/$(SONIC_UTILS) ;\
	  ln -sf $(SONIC)/target/python-debs/$(SONIC_UTILS) $@ ;\
	else \
	  $(MAKE) -C $(SONIC) target/debs/$(SONIC_UTILS) ;\
	  ln -sf $(SONIC)/target/debs/$(SONIC_UTILS) $@ ;\
	fi

python-click_all.deb:
	mkdir -p $(SONIC)/target/debs/stretch
	$(MAKE) -C $(SONIC) target/debs/stretch/$(PYTHON_CLICK)
	ln -sf $(SONIC)/target/debs/stretch/$(PYTHON_CLICK) $@
