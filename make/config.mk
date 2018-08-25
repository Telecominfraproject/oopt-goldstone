include $(ONL)/make/config.mk
#
# Custom configuration here
#
ifndef SONIC
    export SONIC := $(X1)/sm/sonic-buildimage
endif

ifndef sonicadmin_user
    export sonicadmin_user := root
endif
