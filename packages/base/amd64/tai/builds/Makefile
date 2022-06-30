TAI_LIBS := libtai-ldc.so.0.7.0 libtai-menara.so.0.7.0 libtai-lumentum.so.0.7.0 libtai-mux.so.0.7.0 libtai-aco.so

include $(X1)/make/config.amd64.mk
include $(X1)/packages/base/any/tai/builds/Makefile

$(LIB_DIR)/libtai-ldc.so.0.7.0:
	$(call download_library,libtai-ldc:0.7.0)

$(LIB_DIR)/libtai-menara.so.0.7.0:
	$(call download_library,libtai-menara:0.7.0)

$(LIB_DIR)/libtai-lumentum.so.0.7.0:
	$(call download_library,libtai-lumentum:0.7.0)

$(LIB_DIR)/libtai-mux.so.0.7.0:
	$(call download_library,libtai-mux:0.7.0)

$(LIB_DIR)/libtai-aco.so:
	oras pull -o $(LIB_DIR) ghcr.io/oopt-goldstone/libtai-aco:0.1.0@sha256:e27e154de9adcd8be8e80bf453d4bbac63179a7bddc7a1b05451c446963419c3
	mv $(LIB_DIR)/libtai-aco.so.0.1.0 $(LIB_DIR)/libtai-aco.so # libtai-aco.so.0.1.0 doesn't have SO name set
