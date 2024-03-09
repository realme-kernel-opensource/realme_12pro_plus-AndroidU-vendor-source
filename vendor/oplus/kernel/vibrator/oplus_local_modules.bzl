load("//build/kernel/kleaf:kernel.bzl", "ddk_headers")
load("//build/kernel/oplus:oplus_modules_define.bzl", "define_oplus_ddk_module")
load("//build/kernel/oplus:oplus_modules_dist.bzl", "ddk_copy_to_dist_dir")

def define_oplus_local_modules():

    define_oplus_ddk_module(
        name = "oplus_bsp_vibrator_aw8697",
        srcs = native.glob([
            "**/*.h",
            "aw8697_haptic/aw8697.c",
        ]),
        ko_deps = [
            "//vendor/oplus/kernel/vibrator:oplus_bsp_haptic_feedback",
        ],
        includes = ["."],
        local_defines = ["OPLUS_FEATURE_CHG_BASIC","CONFIG_HAPTIC_FEEDBACK_MODULE"],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_vibrator_haptic",
        srcs = native.glob([
            "**/*.h",
            "aw8697_haptic/aw8692x.c",
            "aw8697_haptic/haptic_hv.c",
        ]),
        ko_deps = [
            "//vendor/oplus/kernel/vibrator:oplus_bsp_haptic_feedback",
        ],
        includes = ["."],
        local_defines = ["OPLUS_FEATURE_CHG_BASIC","CONFIG_HAPTIC_FEEDBACK_MODULE"],
    )

    define_oplus_ddk_module(
        name = "oplus_bsp_haptic_feedback",
        srcs = native.glob([
            "**/*.h",
            "aw8697_haptic/haptic_feedback.c",
        ]),
        includes = ["."],
        local_defines = ["OPLUS_FEATURE_CHG_BASIC","CONFIG_HAPTIC_FEEDBACK_MODULE"],
    )

    ddk_copy_to_dist_dir(
        name = "oplus_bsp_vibrator",
        module_list = [
            "oplus_bsp_vibrator_aw8697",
            "oplus_bsp_vibrator_haptic",
            "oplus_bsp_haptic_feedback",
        ],
    )
