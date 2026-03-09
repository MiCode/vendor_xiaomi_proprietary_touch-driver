load(":touch_modules.bzl", "touch_driver_modules")
load(":touch_modules_build.bzl", "define_target_variant_modules")

# this is default list of xiaom touch modules for SM8850
def define_canoe(d,v):
    module_config_options = {
        "xiaomi_touch": ["TOUCH_THP_SUPPORT", "TOUCH_FOD_SUPPORT"],
        "synaptics_tcm2": ["CONFIG_TRUSTED_TOUCH", "TOUCH_THP_SUPPORT", "TOUCH_FOD_SUPPORT"],
    }
    define_target_variant_modules(
        target = "canoe",
        device = d,
        variant = v,
        registry = touch_driver_modules,
        modules = [
            "xiaomi_touch",
            "synaptics_tcm2",
        ],
        module_config_options = module_config_options,
)