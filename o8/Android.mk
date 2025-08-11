ifeq (true, $(strip $(call is-factory-build)))
TOUCH_FACTORY_SELECT:= CONFIG_TOUCH_FACTORY_SELECT=1
endif
FLIP_STATE:= CONFIG_FLIP_STATE=1

KBUILD_OPTIONS := $(TOUCH_FACTORY_SELECT)
KBUILD_OPTIONS += $(FLIP_STATE)
LOCAL_PATH := $(call my-dir)
DLKM_DIR   := device/qcom/common/dlkm

include $(CLEAR_VARS)
LOCAL_SRC_FILES           := $(wildcard $(LOCAL_PATH)/**/*) $(wildcard $(LOCAL_PATH)/*)
LOCAL_MODULE              := xiaomi_touch.ko
LOCAL_MODULE_KBUILD_NAME  := ../xiaomi/xiaomi_touch.ko
LOCAL_MODULE_TAGS         := optional
LOCAL_MODULE_DEBUG_ENABLE := true
LOCAL_MODULE_OWNER := xiaomi
LOCAL_MODULE_PATH         := $(KERNEL_MODULES_OUT)
include $(DLKM_DIR)/Build_external_kernelmodule.mk

include $(CLEAR_VARS)
# For incremental compilation
LOCAL_SRC_FILES           := $(wildcard $(LOCAL_PATH)/**/*) $(wildcard $(LOCAL_PATH)/*)
LOCAL_MODULE              := synaptics_tcm2.ko
LOCAL_MODULE_KBUILD_NAME  := syna_tcm2_pri/synaptics_tcm2.ko
LOCAL_MODULE_TAGS         := optional
LOCAL_MODULE_DEBUG_ENABLE := true
LOCAL_MODULE_PATH         := $(KERNEL_MODULES_OUT)
include $(DLKM_DIR)/Build_external_kernelmodule.mk

include $(CLEAR_VARS)
# For incremental compilation
LOCAL_SRC_FILES           := $(wildcard $(LOCAL_PATH)/**/*) $(wildcard $(LOCAL_PATH)/*)
LOCAL_MODULE              := goodix_core.ko
LOCAL_MODULE_KBUILD_NAME  := goodix_sec/goodix_core.ko
LOCAL_MODULE_TAGS         := optional
LOCAL_MODULE_DEBUG_ENABLE := true
LOCAL_MODULE_PATH         := $(KERNEL_MODULES_OUT)
include $(DLKM_DIR)/Build_external_kernelmodule.mk
