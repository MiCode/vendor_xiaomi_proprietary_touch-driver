load(":touch_modules.bzl", "touch_driver_modules")
load(":touch_modules_build.bzl", "define_target_variant_modules")
load(":target_variants.bzl", "get_all_la_variants", "get_all_le_variants", "get_all_device_variants")
load(":build/canoe.bzl", "define_canoe")
load(":build/pandora.bzl", "define_pandora")
load(":build/popsicle.bzl", "define_popsicle")
load(":build/pudding.bzl", "define_pudding")


def define_touch_target():
    for (t, v) in get_all_la_variants() + get_all_le_variants() + get_all_device_variants():
        if t == "popsicle":
            define_popsicle(t, v)
        elif t == "pandora":
            define_pandora(t, v)
        elif t == "pudding":
            define_pudding(t, v)
        else:
            define_canoe("canoe", v)
