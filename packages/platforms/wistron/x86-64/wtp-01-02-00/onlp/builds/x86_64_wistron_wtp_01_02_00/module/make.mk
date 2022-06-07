###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_wistron_wtp_01_02_00_INCLUDES := -I $(THIS_DIR)inc
x86_64_wistron_wtp_01_02_00_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_wistron_wtp_01_02_00_DEPENDMODULE_ENTRIES := init:x86_64_wistron_wtp_01_02_00 ucli:x86_64_wistron_wtp_01_02_00

