load(":touch_modules.bzl", "touch_driver_modules")
load(":touch_modules_build.bzl", "define_target_variant_modules")

def define_pudding(d,v):
    module_config_options = {
        "xiaomi_touch": [
            "TOUCH_THP_SUPPORT",
            "TOUCH_FOD_SUPPORT"],
        "synaptics_tcm2": [
            "CONFIG_TRUSTED_TOUCH",
            "TOUCH_THP_SUPPORT",
            "TOUCH_FOD_SUPPORT",
            "TOUCH_THERMAL_TEMPERATURE",
            "TOUCH_SPI_CS_CLK_DELAY",
            "TOUCH_SYNA_BOOTLOADER_RECOVERY"],
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