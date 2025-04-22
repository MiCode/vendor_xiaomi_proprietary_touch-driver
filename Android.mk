LOCAL_PATH := $(call my-dir)
ifneq (, $(filter $(call get-miodm-device-name), xuanyuan))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/o2/Android.mk
endif
