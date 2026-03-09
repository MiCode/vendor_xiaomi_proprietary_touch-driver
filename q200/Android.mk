ifeq (true, $(strip $(call is-factory-build)))
TOUCH_FACTORY_SELECT := CONFIG_TOUCH_FACTORY_SELECT=1
endif

$(info get-miodm-device-name = $(call get-miodm-device-name))
ifneq (, $(filter $(call get-miodm-device-name), amethyst))
TOUCH_KBUILD_OPTIONS += CONFIG_TOUCH_THP_SUPPORT=1
TOUCH_KBUILD_OPTIONS += CONFIG_TOUCH_FOD_SUPPORT=1
TOUCH_KBUILD_OPTIONS += CONFIG_CMD_DATA_BUF_SIZE=1
# TOUCH_KBUILD_OPTIONS += CONFIG_TOUCH_KNOCK_SUPPORT=1
# QTS_KBUILD_OPTIONS := CONFIG_TRUSTED_TOUCH_SUPPORT=1
endif

# Build xiaomi_touch.ko
###########################################################
# This is set once per LOCAL_PATH, not per (kernel) module
KBUILD_OPTIONS := TOUCH_ROOT=$(TOUCH_BLD_DIR)
KBUILD_OPTIONS += BOARD_PLATFORM=$(TARGET_BOARD_PLATFORM)
KBUILD_OPTIONS += $(TOUCH_FACTORY_SELECT)
KBUILD_OPTIONS += $(TOUCH_KBUILD_OPTIONS)
# KBUILD_OPTIONS += $(QTS_KBUILD_OPTIONS)
#KBUILD_OPTIONS += KBUILD_EXTRA_SYMBOLS=$(PWD)/$(call intermediates-dir-for,DLKM,msm_drm-module-symvers)/Module.symvers
###########################################################

DLKM_DIR   := device/qcom/common/dlkm

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
# For incremental compilation
LOCAL_MODULE              := xiaomi_touch.ko
LOCAL_MODULE_KBUILD_NAME  := xiaomi_touch.ko
LOCAL_MODULE_DDK_BUILD    := true
LOCAL_MODULE_TAGS         := optional
LOCAL_MODULE_DEBUG_ENABLE := true
LOCAL_MODULE_OWNER := xiaomi
LOCAL_MODULE_PATH         := $(KERNEL_MODULES_OUT)
include $(DLKM_DIR)/Build_external_kernelmodule.mk

include $(CLEAR_VARS)
# For incremental compilation
LOCAL_MODULE              := focaltech_touch.ko
LOCAL_MODULE_KBUILD_NAME  := focaltech_touch.ko
LOCAL_MODULE_DDK_BUILD    := true
LOCAL_MODULE_TAGS         := optional
LOCAL_MODULE_DEBUG_ENABLE := true
LOCAL_MODULE_PATH         := $(KERNEL_MODULES_OUT)
include $(DLKM_DIR)/Build_external_kernelmodule.mk
# Include kp_module.ko in the /vendor/lib/modules (vendor.img)

include $(CLEAR_VARS)
# For incremental compilation
LOCAL_MODULE              := synaptics_tcm2.ko
LOCAL_MODULE_KBUILD_NAME  := synaptics_tcm2.ko
LOCAL_MODULE_DDK_BUILD    := true
LOCAL_MODULE_TAGS         := optional
LOCAL_MODULE_DEBUG_ENABLE := true
LOCAL_MODULE_PATH         := $(KERNEL_MODULES_OUT)
include $(DLKM_DIR)/Build_external_kernelmodule.mk
#

# export to kbuild
# LOCAL_ADDITIONAL_DEPENDENCY := msm_drm.ko
# LOCAL_REQUIRED_MODULES    := msm_drm-module-symvers
# LOCAL_ADDITIONAL_DEPENDENCIES := $(call intermediates-dir-for,DLKM,msm_drm-module-symvers)/Module.symvers
# KBUILD_OPTIONS += KBUILD_EXTRA_SYMBOLS=$(PWD)/$(call intermediates-dir-for,DLKM,msm_drm-module-symvers)/Module.symvers
# $(info KBUILD_OPTIONS = $(KBUILD_OPTIONS))
# $(info intermediates msm_drm symvers path = $(call intermediates-dir-for,DLKM,msm_drm-module-symvers))
# $(info LOCAL_ADDITIONAL_DEPENDENCY = $(LOCAL_ADDITIONAL_DEPENDENCY))
# $(info LOCAL_ADDITIONAL_DEPENDENCIES = $(LOCAL_ADDITIONAL_DEPENDENCIES))
# $(info LOCAL_REQUIRED_MODULES = $(LOCAL_REQUIRED_MODULES))
# $(info DLKM_DIR = $(DLKM_DIR))
#include $(DLKM_DIR)/Build_external_kernelmodule.mk
