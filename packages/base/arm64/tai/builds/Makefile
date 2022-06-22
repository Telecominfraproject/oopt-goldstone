TAI_LIBS := libtai-ldc.so.0.6.0 libtai-menara.so.0.6.0 libtai-lumentum.so.0.6.0 libtai-mux.so.0.6.0

include $(X1)/make/config.arm64.mk
include $(X1)/packages/base/any/tai/builds/Makefile

$(LIB_DIR)/libtai-ldc.so.0.6.0:
	$(call download_library,libtai-ldc:0.6.0)

$(LIB_DIR)/libtai-menara.so.0.6.0:
	$(call download_library,libtai-menara:0.6.0)

$(LIB_DIR)/libtai-lumentum.so.0.6.0:
	$(call download_library,libtai-lumentum:0.6.0)

$(LIB_DIR)/libtai-mux.so.0.6.0:
	$(call download_library,libtai-mux:0.6.0)
