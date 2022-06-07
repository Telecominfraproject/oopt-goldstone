VERSION := 0.5.0
DIGEST := 3987ada8df076e6d7352c8d5212dbd2131b4e3f3c8d9cc79d6600a0fcee87cb2

LIBS := libtai-mdx1.so.$(VERSION) mdx1_fw_app_revb.mem.$(VERSION)

all: $(LIBS)

$(LIBS):
	oras pull ghcr.io/oopt-goldstone/libtai-mdx1:$(VERSION)@sha256:$(DIGEST)
	tar xvf libtai-mdx1_$(VERSION)_linux_arm64.tar
