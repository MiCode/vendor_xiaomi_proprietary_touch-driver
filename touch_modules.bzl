# Importing to touch module entry api from touch_modules_build.bzl to define module entried for touch drivers
load(":touch_modules_build.bzl", "touch_module_entry")

# Importing the touch driver headers defined in BUILD.bazel
touch_driver_modules = touch_module_entry([":touch_drivers_headers"])

#Including the headers in the modules to be declared
module_entry = touch_driver_modules.register

#--------------- TOUCH-DRIVERS MODULES ------------------

#define ddk_module() for goodix_core
module_entry(
    name = "goodix_core",
    config_option = "CONFIG_TOUCHSCREEN_GOODIX_9615V",
    srcs = [
            "p2/goodix/goodix_9615v/goodix_brl_fwupdate.c",
            "p2/goodix/goodix_9615v/goodix_brl_hw.c",
            "p2/goodix/goodix_9615v/goodix_brl_i2c.c",
            "p2/goodix/goodix_9615v/goodix_brl_normalize_coeffi_update.c",
            "p2/goodix/goodix_9615v/goodix_brl_spi.c",
            "p2/goodix/goodix_9615v/goodix_cfg_bin.c",
            "p2/goodix/goodix_9615v/goodix_ts_core.c",
            "p2/goodix/goodix_9615v/goodix_ts_gesture.c",
            "p2/goodix/goodix_9615v/goodix_ts_inspect.c",
            "p2/goodix/goodix_9615v/goodix_ts_tools.c",
            "p2/goodix/goodix_9615v/goodix_ts_utils.c",
    ],
    deps = [
            "xiaomi_touch",
    ],
    copts = [
        # Do nothing
    ]
)

#define ddk_module() for focaltech_3383
module_entry(
    name = "focaltech_touch",
    config_option = "CONFIG_TOUCHSCREEN_FOCALTECH_3383",
    srcs = [
            "q200/focaltech_3383/focaltech_core.c",
            "q200/focaltech_3383/focaltech_ex_fun.c",
            "q200/focaltech_3383/focaltech_ex_mode.c",
            "q200/focaltech_3383/focaltech_gesture.c",
            "q200/focaltech_3383/focaltech_esdcheck.c",
            "q200/focaltech_3383/focaltech_point_report_check.c",
            "q200/focaltech_3383/focaltech_test/focaltech_test.c",
            "q200/focaltech_3383/focaltech_test/focaltech_test_ini.c",
            "q200/focaltech_3383/focaltech_test/supported_ic/focaltech_test_ft3383.c",
            "q200/focaltech_3383/focaltech_flash/focaltech_upgrade_ft3383.c",
            "q200/focaltech_3383/focaltech_flash.c",
            "q200/focaltech_3383/focaltech_spi.c",
    ],
    deps = [
            "xiaomi_touch",
    ],
    copts = [
        # Do nothing
    ]
)

#define ddk_module() for synaptics_tcm2
module_entry(
    name = "synaptics_tcm2",
    config_option = "CONFIG_TOUCHSCREEN_SYNA_TCM2",
    srcs = [
            "p2/syna_tcm2.c",
            "p2/syna_tcm2_cdev.c",
            "p2/tcm/synaptics_touchcom_core_v1.c",
            "p2/tcm/synaptics_touchcom_core_v2.c",
            "p2/tcm/synaptics_touchcom_func_base.c",
            "p2/tcm/synaptics_touchcom_func_touch.c",
            "p2/tcm/synaptics_touchcom_func_reflash.c",
            "p2/tcm/synaptics_touchcom_func_romboot.c",
            "p2/syna_tcm2_platform_spi.c",
            "p2/syna_tcm2_sysfs.c",
            "p2/syna_tcm2_testing.c",
            "p2/syna_xiaomi_driver.c",
    ],
    deps = [
            "xiaomi_touch",
    ],
    kconfig = "config/Kconfig",
    defconfig = "config/touch_p2_consolidate.conf",
    conditional_srcs = {
            "CONFIG_TRUSTED_TOUCH": {
                  True: ["p2/qts/qts_core.c"],
              }
    },
    copts = [
            "-DCONFIG_TOUCHSCREEN_SYNA_TCM2_REFLASH",
            "-DCONFIG_TOUCHSCREEN_SYNA_TCM2_SYSFS",
            "-DCONFIG_TOUCHSCREEN_SYNA_TCM2_TESTING"
    ]
)

#define ddk_module() for xiaomi_touch
module_entry(
    name = "xiaomi_touch",
    config_option = "CONFIG_TOUCHSCREEN_XIAOMITOUCH",
    srcs = [
            "xiaomi/xiaomi_touch_knock_data.c",
            "xiaomi/xiaomi_touch_core.c",
            "xiaomi/xiaomi_touch_proc.c",
            "xiaomi/xiaomi_touch_operations.c",
            "xiaomi/xiaomi_touch_sys.c",
            "xiaomi/xiaomi_touch_mode.c",
            "xiaomi/xiaomi_touch_evdev.c",
            "xiaomi/xiaomi_touch_device.c",
    ],
    copts = [
        # Do nothing
    ]
)