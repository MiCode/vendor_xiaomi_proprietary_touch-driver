LOCAL_PATH := $(call my-dir)

ifneq (, $(filter $(call get-miodm-device-name), darwin))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/darwin/Android.mk
endif
