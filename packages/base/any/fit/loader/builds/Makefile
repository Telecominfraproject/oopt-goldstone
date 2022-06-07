ifndef ARCH
$(error $$ARCH must be set)
endif

.PHONY: x1-loader-fit.itb x1-loader-fit.its

x1-loader-fit.itb: its
	$(ONL)/tools/flat-image-tree.py --initrd x1-loader-initrd:$(ARCH),x1-loader-initrd-$(ARCH).cpio.gz --arch $(ARCH) --add-platform initrd --itb $@
	$(ONLPM) --copy-file x1-loader-initrd:$(ARCH) manifest.json .

x1-loader-fit.its:
	$(ONL)/tools/flat-image-tree.py --initrd x1-loader-initrd:$(ARCH),x1-loader-initrd-$(ARCH).cpio.gz --arch $(ARCH) --add-platform initrd --its $@

its: x1-loader-fit.its
