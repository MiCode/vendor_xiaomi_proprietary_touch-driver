load("//build/kernel/kleaf:kernel.bzl", "ddk_module")

def define_modules(target, variant):
    tv = "{}_{}".format(target, variant)
    copts = []
    deps = ["//msm-kernel:all_headers", ":xiaomi_touch_kernel_headers"]

    ddk_module(
        name = "{}_xiaomi_touch".format(tv),
        out = "xiaomi_touch.ko",
        srcs = native.glob([
            "xiaomi/xiaomi_touch*.c",
            "xiaomi/xiaomi_touch*.h"
        ]),
        hdrs = [":xiaomi_touch_kernel_headers"],
        includes = [".", "linux", "xiaomi"],
        copts = copts,
        deps = deps,
        kernel_build= "//msm-kernel:{}".format(tv),
        visibility = ["//visibility:public"]
    )

print("xiaomi_touch")

