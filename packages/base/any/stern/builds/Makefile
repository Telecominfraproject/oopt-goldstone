STERN_VERSION = 1.20.0

ifndef ARCH
$(error $$ARCH not set)
endif

stern:
	wget -O tmp.tar.gz https://github.com/stern/stern/releases/download/v1.20.0/stern_$(STERN_VERSION)_linux_$(ARCH).tar.gz
	tar xf tmp.tar.gz --strip-components 1
	chmod +x $@
	rm -rf LICENSE tmp.tar.gz
