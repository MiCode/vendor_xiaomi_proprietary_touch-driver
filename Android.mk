LOCAL_PATH := $(call my-dir)
ifneq (, $(filter $(call get-miodm-device-name), haotian))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/o2/Android.mk
endif

ifneq (, $(filter $(call get-miodm-device-name), dada))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/o2/Android.mk
endif

ifneq (, $(filter $(call get-miodm-device-name), xuanyuan))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/o2/Android.mk
endif

ifneq (, $(filter $(call get-miodm-device-name), miro))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/o11u/Android.mk
endif

ifneq (, $(filter $(call get-miodm-device-name), bixi))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/o8/Android.mk
endif

ifneq (, $(filter $(call get-miodm-device-name), nirvana))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/o18/Android.mk
endif

ifneq (, $(filter $(call get-miodm-device-name), luming))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/o9/Android.mk
endif

ifneq (, $(filter $(call get-miodm-device-name), onyx))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/o10u/Android.mk
endif

ifneq (, $(filter $(call get-miodm-device-name), yupei))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/p82/Android.mk
endif

ifneq (, $(filter $(call get-miodm-device-name), piano))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/p81/Android.mk
endif

ifneq (, $(filter $(call get-miodm-device-name), annibale))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/p11/Android.mk
endif

ifneq (, $(filter $(call get-miodm-device-name), warsaw))
$(info "touch driver build for $(call get-miodm-device-name)")
include $(LOCAL_PATH)/warsaw/Android.mk
endif