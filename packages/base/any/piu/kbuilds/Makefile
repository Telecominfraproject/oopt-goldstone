THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

export MODULE_DIR := $(THIS_DIR)

ifndef OUTPUT_DIR
$(error $$OUTPUT_DIR must by the caller)
endif

ifndef MODULE_DIR
$(error $$MODULE_DIR not set)
endif

ifndef KERNEL
$(error $$KERNEL not set)
endif

ifndef ARCH
$(error $$ARCH not set)
endif

KBUILD := $(shell $(ONL)/tools/onlpm.py --find-dir $(KERNEL) mbuilds)

ifndef INSTALL_MOD_PATH
INSTALL_MOD_PATH := $(OUTPUT_DIR)
endif

ifndef INSTALL_MOD_DIR
INSTALL_MOD_DIR := "onl/onl/common"
endif

build:
	$(RM) -r $(OUTPUT_DIR)/lib
	test -d $(KBUILD)
	$(MAKE) -C $(KBUILD) M=$(MODULE_DIR) modules
	$(MAKE) -C $(KBUILD) M=$(MODULE_DIR) INSTALL_MOD_PATH=$(INSTALL_MOD_PATH) INSTALL_MOD_DIR=$(INSTALL_MOD_DIR) modules_install
	$(RM) $(shell find $(OUTPUT_DIR)/lib -name \*modules\* -type f)

clean:
	$(MAKE) -C $(KBUILD) M=$(MODULE_DIR) clean
