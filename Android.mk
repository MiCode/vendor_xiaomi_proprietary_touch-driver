LOCAL_PATH := $(call my-dir)
ifneq (, $(filter $(call get-miodm-device-name), popsicle))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/p2/Android.mk
endif

ifneq (, $(filter $(call get-miodm-device-name), pudding))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/p2/Android.mk
endif

ifneq (, $(filter $(call get-miodm-device-name), pandora))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/q200/Android.mk
endif
