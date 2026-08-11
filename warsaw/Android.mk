ifeq (true, $(strip $(call is-factory-build)))
TOUCH_FACTORY_SELECT := CONFIG_TOUCH_FACTORY_SELECT=1
endif

$(info get-miodm-device-name = $(call get-miodm-device-name))
ifneq (, $(filter $(call get-miodm-device-name), warsaw))
QTS_KBUILD_OPTIONS := CONFIG_TRUSTED_TOUCH=1
TOUCH_KBUILD_OPTIONS += CONFIG_TOUCH_THP_SUPPORT=1
TOUCH_KBUILD_OPTIONS += CONFIG_TOUCH_FOD_SUPPORT=1
# TOUCH_KBUILD_OPTIONS += CONFIG_TOUCH_KNOCK_SUPPORT=1
TOUCH_KBUILD_OPTIONS += CONFIG_THP_V4_ENABLE=1
TOUCH_KBUILD_OPTIONS += CONFIG_TOUCH_SPI_CS_CLK_DELAY=1
endif

# This makefile is only for DLKM
ifneq ($(findstring vendor,$(LOCAL_PATH)),)

# ifneq ($(findstring opensource,$(LOCAL_PATH)),)
# 	TOUCH_BLD_DIR := $(TOP)/vendor/xiaomi/proprietary/touch/touchfeature_v2/touch_driver/o11u
# 	$(info wqk TOUCH_BLD_DIR = $(TOUCH_BLD_DIR))
# endif

# Build xiaomi_touch.ko
###########################################################
KBUILD_OPTIONS := TOUCH_ROOT=$(TOUCH_BLD_DIR)
KBUILD_OPTIONS += BOARD_PLATFORM=$(TARGET_BOARD_PLATFORM)
KBUILD_OPTIONS += $(TOUCH_FACTORY_SELECT)
KBUILD_OPTIONS += $(TOUCH_KBUILD_OPTIONS)
KBUILD_OPTIONS += $(QTS_KBUILD_OPTIONS)
###########################################################

DLKM_DIR   := device/qcom/common/dlkm

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
# For incremental compilation
LOCAL_SRC_FILES           := $(wildcard $(LOCAL_PATH)/**/*) $(wildcard $(LOCAL_PATH)/*)
LOCAL_MODULE              := xiaomi_touch.ko
LOCAL_MODULE_DDK_BUILD    := true
LOCAL_MODULE_KBUILD_NAME  := xiaomi/xiaomi_touch.ko
LOCAL_MODULE_TAGS         := optional
LOCAL_MODULE_DEBUG_ENABLE := true
LOCAL_MODULE_OWNER := xiaomi
LOCAL_MODULE_PATH         := $(KERNEL_MODULES_OUT)
# Include kp_module.ko in the /vendor/lib/modules (vendor.img)
# BOARD_VENDOR_KERNEL_MODULES += $(LOCAL_MODULE_PATH)/$(LOCAL_MODULE)
include $(DLKM_DIR)/Build_external_kernelmodule.mk

include $(CLEAR_VARS)
# For incremental compilation
LOCAL_SRC_FILES           := $(wildcard $(LOCAL_PATH)/**/*) $(wildcard $(LOCAL_PATH)/*)
LOCAL_MODULE              := goodix_core.ko
LOCAL_MODULE_DDK_BUILD    := true
LOCAL_MODULE_KBUILD_NAME  := goodix_9916k/goodix_core.ko 
LOCAL_MODULE_TAGS         := optional
LOCAL_MODULE_DEBUG_ENABLE := true
LOCAL_MODULE_PATH         := $(KERNEL_MODULES_OUT)
include $(DLKM_DIR)/Build_external_kernelmodule.mk
endif
