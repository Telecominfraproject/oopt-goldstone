include $(X1)/make/config.mk

PACKAGES := x1-swi x1-installer-installed
ARCHES := amd64

all:
	rm -rf *.swi *INSTALLER
	$(foreach arch,$(ARCHES),$(foreach package,$(PACKAGES),$(ONLPM) --extract-dir $(package):$(arch) .;))
	$(ONLPM) --extract-dir onl-mibs:all .
	find . -name "*.swi" -exec mv {} . \;
	find . -name "*INSTALLER" -exec mv {} . \;
	$(ONL)/tools/sjson.py --in whitelist.yml > hcl-whitelist.json
