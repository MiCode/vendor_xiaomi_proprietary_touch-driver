LOCAL_PATH := $(call my-dir)

ifneq (, $(filter $(call get-miodm-device-name), bixi))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/o8/Android.mk
endif