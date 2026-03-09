load(":touch_modules.bzl", "touch_driver_modules")
load(":touch_modules_build.bzl", "define_target_variant_modules")

def define_pandora(d,v):
    #for user version
    module_config_options_perf = {
        "xiaomi_touch": [
            "TOUCH_THP_SUPPORT", 
            "TOUCH_FOD_SUPPORT",
            "TOUCH_MULTI_PANEL_NOTIFIER_SUPPORT",
        ],
        "synaptics_tcm2": [
            "CONFIG_TRUSTED_TOUCH", 
            "TOUCH_THP_SUPPORT", 
            "TOUCH_FOD_SUPPORT", 
            "TOUCH_THERMAL_TEMPERATURE", 
            "TOUCH_SPI_CS_CLK_DELAY", 
            "TOUCH_SYNA_BOOTLOADER_RECOVERY",
            "TOUCH_MULTI_PANEL_NOTIFIER_SUPPORT",
        ],
        "focaltech_touch": [
            "TOUCH_THP_SUPPORT", 
            "TOUCH_FOD_SUPPORT", 
            "TOUCH_GESTURE_ALWAYSON_SUPPORT",
            "TOUCH_MULTI_PANEL_NOTIFIER_SUPPORT", 
        ],
    }
    #for userdebug version
    module_config_options_consolidate = {
        "xiaomi_touch": [
            "TOUCH_THP_SUPPORT", 
            "TOUCH_FOD_SUPPORT",
            "TOUCH_MULTI_PANEL_NOTIFIER_SUPPORT",
        ],
        "synaptics_tcm2": [
            "CONFIG_TRUSTED_TOUCH", 
            "TOUCH_THP_SUPPORT", 
            "TOUCH_FOD_SUPPORT", 
            "TOUCH_THERMAL_TEMPERATURE", 
            "TOUCH_SPI_CS_CLK_DELAY", 
            "TOUCH_SYNA_BOOTLOADER_RECOVERY",
            "TOUCH_MULTI_PANEL_NOTIFIER_SUPPORT",
        ],
        "focaltech_touch": [
            "TOUCH_THP_SUPPORT", 
            "TOUCH_FOD_SUPPORT", 
            "USER_DEBUG_BUILD", 
            "TOUCH_GESTURE_ALWAYSON_SUPPORT",
            "TOUCH_MULTI_PANEL_NOTIFIER_SUPPORT", 
        ],
    }

    if v == "consolidate":
        module_config_options = module_config_options_consolidate
    else:
        module_config_options = module_config_options_perf
    define_target_variant_modules(
        target = "canoe",
        device = d,
        variant = v,
        registry = touch_driver_modules,
        modules = [
            "xiaomi_touch",
            "synaptics_tcm2",
            "focaltech_touch",
        ],
        module_config_options = module_config_options,
)