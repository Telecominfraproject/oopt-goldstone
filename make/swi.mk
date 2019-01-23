ONL_PRE_SWITOOL_CMDS := $(X1)/tools/zerotouch.py --manifest-version 1 --operation swi --manifest manifest.json > zerotouch.json
ONL_SWITOOL_EXTRA_ARGS := --add-files zerotouch.json
include $(ONL)/make/swi.mk
