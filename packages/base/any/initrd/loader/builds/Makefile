############################################################
#
# X1  Loader initrd build template.
#
############################################################
ifndef ARCH
  $(error $$ARCH must be set)
endif

ROOT := root
TARGET := x1-loader-initrd-$(ARCH).cpio.gz
.PHONY: $(TARGET)

$(TARGET):
	$(ONLPM) --copy-file onl-loader-initrd:$(ARCH) onl-loader-initrd-$(ARCH).cpio.gz .
	$(ONLPM) --copy-file onl-loader-initrd:$(ARCH) manifest.json .
	$(ONL)/tools/sjson.py --inout manifest.json --kj version $(X1)/make/versions/version-sonl.json
	$(X1)/tools/zerotouch.py --manifest-version 1 --operation installer --manifest manifest.json > zerotouch.json
	sudo rm -rf $(ROOT) && mkdir $(ROOT)
	sudo mkdir -p $(ROOT)/etc/sonl/loader && sudo cp manifest.json $(ROOT)/etc/sonl/loader
	sudo cp $(X1)/make/versions/version-sonl.json $(ROOT)/etc/sonl/loader/versions.json
	sudo cp $(X1)/make/versions/version-sonl.sh   $(ROOT)/etc/sonl/loader/versions.sh
	sudo cp $(X1)/make/versions/version-sonl.mk   $(ROOT)/etc/sonl/loader/versions.mk
	$(ONLPM) --sudo --force --extract-dir x1-sysconfig:all $(ROOT)
	$(ONLPM) --sudo --force --extract-dir x1-loader-files:all $(ROOT)
	sudo $(ONL)/tools/cpiomod.py --cpio onl-loader-initrd-$(ARCH).cpio.gz --add-directory $(ROOT) --out $@
	sudo rm -rf $(ROOT) onl-loader-initrd-$(ARCH).cpio.gz
