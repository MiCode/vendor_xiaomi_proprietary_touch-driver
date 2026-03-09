load("//build/kernel/kleaf:kernel.bzl", "ddk_module", "ddk_submodule", "kernel_module_group")
load("//build/bazel_common_rules/dist:dist.bzl", "copy_to_dist_dir")

def _create_module_conditional_src_map(conditional_srcs):
    processed_conditional_srcs = {}

    for conditional_src_name in conditional_srcs:
        conditional_src = conditional_srcs[conditional_src_name]

        if type(conditional_src) == "list":
            processed_conditional_srcs[conditional_src_name] = { True: conditional_src }
        else:
            processed_conditional_srcs[conditional_src_name] = conditional_src

    return processed_conditional_srcs

def _register_module_to_map(module_map, name, path, config_option, srcs, deps, kconfig, defconfig, conditional_srcs, copts):

    module = struct(
        name = name,
        path = path,
        srcs = srcs,
        deps = deps,
        config_option = config_option,
        kconfig = kconfig,
        defconfig = defconfig,
        conditional_srcs = conditional_srcs,
        copts = copts,
    )

    module_map[name] = module

def _get_config_choices(map, options):
    choices = []
    for option in map:
        choices.extend(map[option].get(option in options, []))
    return choices

def _get_kernel_build_options(modules, config_options):
    all_options = {option: True for option in config_options}
    all_options = all_options | {module.config_option: True for module in modules if module.config_option}
    return all_options

def _get_kernel_build_module_srcs(module, formatter):
    srcs = module.srcs
    print("-", module.name, ",", module.config_option, ",srcs =", srcs)
    module_path = "{}/".format(module.path) if module.path else ""
    return ["{}{}".format(module_path, formatter(src)) for src in srcs]

def _get_kernel_build_module_deps(module, formatter):
    deps = []
    if module.deps:
        for dep in module.deps:
            deps.append(formatter(dep))
    return deps

def touch_module_entry(hdrs = []):
    module_map = {}

    def register(name, path = None, config_option = [], srcs = [], config_srcs = None, deps = None, kconfig = None, defconfig = None, conditional_srcs = None, copts = []):
        _register_module_to_map(module_map, name, path, config_option, srcs, deps, kconfig, defconfig, conditional_srcs, copts)

    return struct(
        register = register,
        get = module_map.get,
        hdrs = hdrs,
    )

def define_target_variant_modules(target, device, variant, registry, modules, module_config_options = {}):
    kernel_build = "{}_{}".format(device, variant)
    print("touch Defining kernel build: ", kernel_build)
    kernel_build_label = select({
        "//build/kernel/kleaf:socrepo_true": "//soc-repo:{}_base_kernel".format(kernel_build),
        "//build/kernel/kleaf:socrepo_false": "//msm-kernel:{}".format(kernel_build),
    })
    modules = [registry.get(module_name) for module_name in modules]
    build_print = lambda message: print("{}: {}".format(kernel_build, message))
    formatter = lambda s: s.replace("%b", kernel_build).replace("%t", target).replace("%d", device).replace("%v", variant)
    deps = select({
        "//build/kernel/kleaf:socrepo_true": [
            "//soc-repo:all_headers",
            "//soc-repo:{}/drivers/pinctrl/qcom/pinctrl-msm".format(kernel_build),
            "//soc-repo:{}/drivers/soc/qcom/panel_event_notifier".format(kernel_build),
            "//soc-repo:{}/drivers/virt/gunyah/gh_mem_notifier".format(kernel_build),
            "//soc-repo:{}/drivers/virt/gunyah/gh_irq_lend".format(kernel_build),
            "//soc-repo:{}/drivers/virt/gunyah/gh_rm_drv".format(kernel_build),
            "//soc-repo:{}/drivers/misc/miev".format(kernel_build),
        ],
        "//build/kernel/kleaf:socrepo_false": ["//msm-kernel:all_headers"],
    })

    # xiaomi defined factory micro
    factory_copts = select({
        ":factory_build_v1": [
            "-DFACTORY_BUILD=1",
            "-DCONFIG_TOUCH_FACTORY_BUILD",
            "-DCONFIG_FACTORY_BUILD=1",
        ],
        "//conditions:default": [
            # Do nothing
        ],
    })

    all_module_rules = []

    for module in modules:
        module_dep = []
        rule_name = "{}_{}".format(kernel_build, module.name)
        module_srcs = _get_kernel_build_module_srcs(module, formatter)

        if not module_srcs:
            continue

        config_options = module_config_options.get(module.name, [])
        options = _get_kernel_build_options([module], config_options)
        print("module.name: ", module.name, ", options: ", options)

        module_deps = _get_kernel_build_module_deps(module, formatter)
        if module_deps:
            for dep in module_deps:
                if dep.startswith("//"):
                        module_dep.append(dep)
                else:
                    module_dep.append("{}_{}".format(kernel_build, dep))
        print("module_dep: ", module_dep)

        ddk_module(
            name = rule_name,
            srcs = module_srcs,
            out = "{}.ko".format(module.name),
            kernel_build = kernel_build_label,
            deps = deps + module_dep + registry.hdrs,
            local_defines = options.keys(),
            kconfig = module.kconfig,
            defconfig = module.defconfig,
            conditional_srcs = module.conditional_srcs,
            copts = module.copts + factory_copts,
            visibility = ["//visibility:public"],
        )

        all_module_rules.append(rule_name)

    kernel_module_group(
        name = "{}_touch_modules".format(kernel_build),
        srcs = all_module_rules,
    )
    copy_to_dist_dir(
        name = "{}_touch_drivers_dist".format(kernel_build),
        data = [":{}_touch_modules".format(kernel_build)],
        dist_dir = "out/target/product/{}/dlkm/lib/modules".format(device),
        flat = True,
        wipe_dist_dir = False,
        allow_duplicate_filenames = False,
        mode_overrides = {"**/*": "644"},
        log = "debug",
    )

def define_consolidate_gki_modules(target, registry, modules, config_options = []):
    define_target_variant_modules(target, "gki", registry, modules, config_options)
    define_target_variant_modules(target, "consolidate", registry, modules, config_options)
