TAI_LIBS := libtai-ldc.so.0.5.0 libtai-menara.so.0.5.0 libtai-lumentum.so.0.5.0 libtai-mux.so.0.5.0

include $(X1)/make/config.arm64.mk
include $(X1)/packages/base/any/tai/builds/Makefile

$(LIB_DIR)/libtai-ldc.so.0.5.0:
	oras pull -o $(LIB_DIR) ghcr.io/oopt-goldstone/libtai-ldc:0.5.0@sha256:b9c8614170fe777574d02827a043b89cb30a292141440513cb74be3dc028c578

$(LIB_DIR)/libtai-menara.so.0.5.0:
	oras pull -o $(LIB_DIR) ghcr.io/oopt-goldstone/libtai-menara:0.5.0@sha256:d17c36374ecd510f589289729d93f9d71e4966e0f7e7f8fb2edf9f2cfe9ca37f

$(LIB_DIR)/libtai-lumentum.so.0.5.0:
	oras pull -o $(LIB_DIR) ghcr.io/oopt-goldstone/libtai-lumentum:0.5.0@sha256:38940e833c76eb8765555e32cf3fd97c0afd83097ca25b3dd544878e7b875c80

$(LIB_DIR)/libtai-mux.so.0.5.0:
	oras pull -o $(LIB_DIR) ghcr.io/oopt-goldstone/libtai-mux:0.5.0@sha256:083dea313596db52b7485fdb62d690d23b8f87817957d970a7037341292f455b
