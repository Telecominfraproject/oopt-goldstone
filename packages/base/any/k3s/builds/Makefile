all: k3s k3s-airgap-images-$(ARCH).tar.gz

k3s:
	oras pull ghcr.io/oopt-goldstone/k3s:1.22.2@sha256:$(K3S_DIGEST)
	tar xvf k3s.tar

k3s-airgap-images-$(ARCH).tar.gz: k3s
