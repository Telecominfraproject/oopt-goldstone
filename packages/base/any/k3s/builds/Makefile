K3S_VERSION = v1.18.3

all: k3s-airgap-images-$(ARCH).tar k3s install.sh

k3s-airgap-images-$(ARCH).tar:
	wget -O $@ https://github.com/rancher/k3s/releases/download/$(K3S_VERSION)%2Bk3s1/k3s-airgap-images-$(ARCH).tar

k3s:
	wget -O $@ https://github.com/rancher/k3s/releases/download/$(K3S_VERSION)%2Bk3s1/k3s
	chmod +x $@

install.sh:
	wget -O $@ https://raw.githubusercontent.com/rancher/k3s/master/install.sh
	chmod +x $@
