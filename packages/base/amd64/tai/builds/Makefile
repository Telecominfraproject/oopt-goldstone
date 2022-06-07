TAI_LIBS := libtai-ldc.so.0.5.0 libtai-menara.so.0.5.0 libtai-lumentum.so.0.5.0 libtai-mux.so.0.5.0 libtai-aco.so.0.1.0

include $(X1)/make/config.amd64.mk
include $(X1)/packages/base/any/tai/builds/Makefile

$(LIB_DIR)/libtai-ldc.so.0.5.0:
	oras pull -o $(LIB_DIR) ghcr.io/oopt-goldstone/libtai-ldc:0.5.0@sha256:34b7953204e2c92988bd20c4f4c40db5a8ebe628b2820b5d28ac71f1672f6c1b

$(LIB_DIR)/libtai-menara.so.0.5.0:
	oras pull -o $(LIB_DIR) ghcr.io/oopt-goldstone/libtai-menara:0.5.0@sha256:d18a51440b9b98a8a029e7bfced83c68d363bfeca4c2a52ec53a98ff22695420

$(LIB_DIR)/libtai-lumentum.so.0.5.0:
	oras pull -o $(LIB_DIR) ghcr.io/oopt-goldstone/libtai-lumentum:0.5.0@sha256:4273a2ed6ae1b80b568c20487faea391c07e12ff849a882ecd8d11c3fcff7b67

$(LIB_DIR)/libtai-mux.so.0.5.0:
	oras pull -o $(LIB_DIR) ghcr.io/oopt-goldstone/libtai-mux:0.5.0@sha256:c72f6d40d57f14823471b2a472ff2216525d7d8b347b33625a01911d8cd26028

$(LIB_DIR)/libtai-aco.so.0.1.0:
	oras pull -o $(LIB_DIR) ghcr.io/oopt-goldstone/libtai-aco:0.1.0@sha256:e27e154de9adcd8be8e80bf453d4bbac63179a7bddc7a1b05451c446963419c3
