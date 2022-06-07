###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
arm64_wistron_wtp_01_c1_00_INCLUDES := -I $(THIS_DIR)inc
arm64_wistron_wtp_01_c1_00_INTERNAL_INCLUDES := -I $(THIS_DIR)src
arm64_wistron_wtp_01_c1_00_DEPENDMODULE_ENTRIES := init:arm64_wistron_wtp_01_c1_00 ucli:arm64_wistron_wtp_01_c1_00

