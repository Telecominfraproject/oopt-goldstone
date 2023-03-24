SWSS_COMMON_DIR := ../src/sonic-swss-common

all:
	cd $(SWSS_COMMON_DIR) && dpkg-buildpackage -us -uc -b && cd ../ && dpkg -i *.deb
