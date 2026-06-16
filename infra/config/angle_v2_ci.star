# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""CI ANGLE builders using the angle_v2 recipe."""

load("@chromium-luci//builder_config.star", "builder_config")
load("@chromium-luci//ci.star", "ci")
load("@chromium-luci//consoles.star", "consoles")
load("@chromium-luci//gardener_rotations.star", "gardener_rotations")
load("@chromium-luci//gn_args.star", "gn_args")
load("@chromium-luci//targets.star", "targets")
load("//angle_v2_shared.star", "builder_defaults")
load("//constants.star", "default_experiments", "siso")

ci.defaults.set(
    executable = "recipe:angle_v2/angle_v2",
    builder_group = "ci",
    bucket = "ci",
    pool = "luci.chromium.gpu.ci",
    triggered_by = ["main-poller"],
    build_numbers = True,
    contact_team_email = "chrome-gpu-infra@google.com",
    gardener_rotations = gardener_rotations.rotation("angle", None, None),
    service_account = "angle-ci-builder@chops-service-accounts.iam.gserviceaccount.com",
    shadow_service_account = "angle-try-builder@chops-service-accounts.iam.gserviceaccount.com",
    siso_project = siso.project.DEFAULT_TRUSTED,
    shadow_siso_project = siso.project.DEFAULT_UNTRUSTED,
    siso_remote_jobs = siso.remote_jobs.DEFAULT,
    thin_tester_cores = 2,
    builderless = True,
    experiments = default_experiments,
    test_presentation = resultdb.test_presentation(
        column_keys = ["v.gpu"],
        grouping_keys = ["status", "v.test_suite"],
    ),
)

targets.builder_defaults.set(
    mixins = [
        "chromium-tester-service-account",
        "swarming_containment_auto",
    ],
)

################################################################################
# Parent Builders                                                              #
################################################################################

def angle_linux_parent_builder(**kwargs):
    kwargs = builder_defaults.apply_linux_builder_defaults(kwargs)
    ci.builder(**kwargs)

def angle_mac_parent_builder(**kwargs):
    kwargs = builder_defaults.apply_mac_builder_defaults(kwargs)
    ci.builder(**kwargs)

def angle_win_parent_builder(**kwargs):
    kwargs = builder_defaults.apply_win_clang_builder_defaults(kwargs)
    ci.builder(**kwargs)

def angle_win_msvc_parent_builder(**kwargs):
    kwargs = builder_defaults.apply_win_msvc_builder_defaults(kwargs)
    ci.builder(**kwargs)

angle_linux_parent_builder(
    name = "angle-android-arm-builder-dbg",
    description_html = "Compiles debug ANGLE test binaries for Android/arm",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_android",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.DEBUG,
            target_arch = builder_config.target_arch.ARM,
            target_bits = 32,
            target_platform = builder_config.target_platform.ANDROID,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "android_clang",
            "android_static_analysis",
            "arm",
            "component",
            "debug",
            "opencl",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|android|arm",
        short_name = "dbg",
    ),
)

angle_linux_parent_builder(
    name = "angle-android-arm-builder-rel",
    description_html = "Compiles release ANGLE test binaries for Android/arm",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_android",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.ARM,
            target_bits = 32,
            target_platform = builder_config.target_platform.ANDROID,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "android_clang",
            "android_static_analysis",
            "arm",
            "capture",
            "component",
            "opencl",
            "release_with_dchecks",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|android|arm",
        short_name = "rel",
    ),
)

angle_linux_parent_builder(
    name = "angle-android-arm64-builder-dbg",
    description_html = "Compiles debug ANGLE test binaries for Android/arm64",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_android",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.DEBUG,
            target_arch = builder_config.target_arch.ARM,
            target_bits = 64,
            target_platform = builder_config.target_platform.ANDROID,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "android_clang",
            "android_static_analysis",
            "arm64",
            "component",
            "debug",
            "opencl",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|android|arm64",
        short_name = "dbg",
    ),
)

angle_linux_parent_builder(
    name = "angle-android-arm64-builder-perf",
    description_html = "Compiles release ANGLE perf test binaries for Android/arm64",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_android",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.ARM,
            target_bits = 64,
            target_platform = builder_config.target_platform.ANDROID,
        ),
        perf_isolate_upload = True,
    ),
    gn_args = gn_args.config(
        configs = [
            "android_clang",
            "android_static_analysis",
            "arm64",
            "component",
            "dcheck_off",
            "release",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "perf|android|arm64",
        short_name = "bld",
    ),
)

angle_linux_parent_builder(
    name = "angle-android-arm64-builder-rel",
    description_html = "Compiles release ANGLE test binaries for Android/arm64",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_android",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.ARM,
            target_bits = 64,
            target_platform = builder_config.target_platform.ANDROID,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "android_clang",
            "android_static_analysis",
            "arm64",
            "capture",
            "component",
            "opencl",
            "release_with_dchecks",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|android|arm64",
        short_name = "rel",
    ),
)

angle_linux_parent_builder(
    name = "angle-linux-x64-builder-asan",
    description_html = ("Compiles release ANGLE test binaries for Linux/x64 " +
                        "with ASan, LSan, and UBSan enabled"),
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "asan",
            "component",
            "linux_clang",
            "lsan",
            "opencl",
            "release_with_dchecks",
            "smoke_traces",
            "ubsan",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|linux|x64",
        short_name = "asn",
    ),
)

angle_linux_parent_builder(
    name = "angle-linux-x64-builder-dbg",
    description_html = "Compiles debug ANGLE test binaries for Linux/x64",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.DEBUG,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "component",
            "debug",
            "linux_clang",
            "opencl",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|linux|x64",
        short_name = "dbg",
    ),
)

angle_linux_parent_builder(
    name = "angle-linux-x64-builder-perf",
    description_html = "Compiles release ANGLE perf test binaries for Linux/x64",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
        perf_isolate_upload = True,
    ),
    gn_args = gn_args.config(
        configs = [
            "component",
            "dcheck_off",
            "linux_clang",
            "release",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "perf|linux|x64",
        short_name = "bld",
    ),
)

angle_linux_parent_builder(
    name = "angle-linux-x64-builder-rel",
    description_html = "Compiles release ANGLE test binaries for Linux/x64",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "capture",
            "component",
            "linux_clang",
            "opencl",
            "release_with_dchecks",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|linux|x64",
        short_name = "rel",
    ),
)

angle_linux_parent_builder(
    name = "angle-linux-x64-builder-tsan",
    description_html = "Compiles release ANGLE test binaries for Linux/x64 with TSan enabled",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "component",
            "linux_clang",
            "opencl",
            "release_with_dchecks",
            "tsan",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|linux|x64",
        short_name = "tsn",
    ),
)

angle_mac_parent_builder(
    name = "angle-mac-arm64-builder-rel",
    description_html = "Compiles release ANGLE test binaries for Mac/arm64",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.ARM,
            target_bits = 64,
            target_platform = builder_config.target_platform.MAC,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "arm64",
            "capture",
            "component",
            "mac_clang",
            "release_with_dchecks",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|mac|arm64",
        short_name = "rel",
    ),
)

angle_mac_parent_builder(
    name = "angle-mac-x64-builder-dbg",
    description_html = "Compiles debug ANGLE test binaries for Mac/x64",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.DEBUG,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.MAC,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "component",
            "debug",
            "mac_clang",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|mac|x64",
        short_name = "dbg",
    ),
)

angle_mac_parent_builder(
    name = "angle-mac-x64-builder-rel",
    description_html = "Compiles release ANGLE test binaries for Mac/x64",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.MAC,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "capture",
            "component",
            "mac_clang",
            "release_with_dchecks",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|mac|x64",
        short_name = "rel",
    ),
)

angle_win_parent_builder(
    name = "angle-win-x64-builder-asan",
    description_html = "Compiles release ANGLE test binaries for Win/x64 with ASan enabled",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "asan",
            "component",
            "opencl",
            "release_with_dchecks",
            "win_clang",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|win|x64",
        short_name = "asn",
    ),
)

angle_win_parent_builder(
    name = "angle-win-x64-builder-dbg",
    description_html = "Compiles debug ANGLE test binaries for Win/x64",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.DEBUG,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "component",
            "debug",
            "opencl",
            "win_clang",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|win|x64",
        short_name = "dbg",
    ),
)

angle_win_msvc_parent_builder(
    name = "angle-win-x64-builder-msvc-dbg",
    description_html = "Compiles debug ANGLE targets for Win/x64 using MSVC",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.DEBUG,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "component",
            "debug",
            "win_msvc",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|win|x64|msvc",
        short_name = "dbg",
    ),
)

angle_win_msvc_parent_builder(
    name = "angle-win-x64-builder-msvc-rel",
    description_html = "Compiles release ANGLE targets for Win/x64 using MSVC",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "component",
            "release_with_dchecks",
            "win_msvc",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|win|x64|msvc",
        short_name = "rel",
    ),
)

angle_win_parent_builder(
    name = "angle-win-x64-builder-perf",
    description_html = "Compiles release ANGLE perf test binaries for Win/x64",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
        perf_isolate_upload = True,
    ),
    gn_args = gn_args.config(
        configs = [
            "component",
            "dcheck_off",
            "release",
            "win_clang",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "perf|win|x64",
        short_name = "bld",
    ),
)

angle_win_parent_builder(
    name = "angle-win-x64-builder-rel",
    description_html = "Compiles release ANGLE test binaries for Win/x64",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "capture",
            "component",
            "opencl",
            "release_with_dchecks",
            "win_clang",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|win|x64",
        short_name = "rel",
    ),
)

angle_win_msvc_parent_builder(
    name = "angle-win-x64-builder-uwp-dbg",
    description_html = "Compiles debug ANGLE targets for Windows UWP/x64 using MSVC",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.DEBUG,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "component",
            "debug",
            "win_uwp",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|win|x64|uwp",
        short_name = "dbg",
    ),
)

angle_win_msvc_parent_builder(
    name = "angle-win-x64-builder-uwp-rel",
    description_html = "Compiles release ANGLE targets for Windows UWP/x64 using MSVC",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "component",
            "release_with_dchecks",
            "win_uwp",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|win|x64|uwp",
        short_name = "rel",
    ),
)

angle_win_parent_builder(
    name = "angle-win-x86-builder-dbg",
    description_html = "Compiles debug ANGLE test binaries for Win/x86",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.DEBUG,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 32,
            target_platform = builder_config.target_platform.WIN,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "component",
            "debug",
            "opencl",
            "win_clang",
            "x86",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|win|x86",
        short_name = "dbg",
    ),
)

angle_win_msvc_parent_builder(
    name = "angle-win-x86-builder-msvc-dbg",
    description_html = "Compiles debug ANGLE targets for Win/x86 using MSVC",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.DEBUG,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 32,
            target_platform = builder_config.target_platform.WIN,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "component",
            "debug",
            "win_msvc",
            "x86",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|win|x86|msvc",
        short_name = "dbg",
    ),
)

angle_win_msvc_parent_builder(
    name = "angle-win-x86-builder-msvc-rel",
    description_html = "Compiles release ANGLE targets for Win/x86 using MSVC",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 32,
            target_platform = builder_config.target_platform.WIN,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "component",
            "release_with_dchecks",
            "win_msvc",
            "x86",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|win|x86|msvc",
        short_name = "rel",
    ),
)

angle_win_parent_builder(
    name = "angle-win-x86-builder-rel",
    description_html = "Compiles release ANGLE test binaries for Win/x86",
    schedule = "triggered",
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 32,
            target_platform = builder_config.target_platform.WIN,
        ),
    ),
    gn_args = gn_args.config(
        configs = [
            "component",
            "opencl",
            "release_with_dchecks",
            "win_clang",
            "x86",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "compile|win|x86",
        short_name = "rel",
    ),
)

################################################################################
# Child Testers                                                                #
################################################################################

ci.thin_tester(
    name = "angle-android-arm64-google-pixel4-perf",
    description_html = "Perf tests release ANGLE on Android/arm64 on Pixel 4 devices",
    parent = "angle-android-arm64-builder-perf",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_android",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.ARM,
            target_bits = 64,
            target_platform = builder_config.target_platform.ANDROID,
        ),
        run_tests_serially = True,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "perf|android|arm64",
        short_name = "p4",
    ),
)

ci.thin_tester(
    name = "angle-android-arm64-google-pixel4-rel",
    description_html = "Tests release ANGLE on Android/arm64 on Pixel 4 devices",
    parent = "angle-android-arm64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_android",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.ARM,
            target_bits = 64,
            target_platform = builder_config.target_platform.ANDROID,
        ),
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [
            "android_gl_and_gles_gtests",
            "android_no_vulkan_gtests",
            "android_p4_isolated_scripts",
        ],
        mixins = [
            "gpu_pixel_4_stable",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.ANDROID,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|android|arm64|rel",
        short_name = "p4",
    ),
)

ci.thin_tester(
    name = "angle-android-arm64-google-pixel6-exp-rel",
    description_html = "Tests release ANGLE on Android/arm64 on experimental Pixel 6 devices",
    parent = "angle-android-arm64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_android",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.ARM,
            target_bits = 64,
            target_platform = builder_config.target_platform.ANDROID,
        ),
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [
            "android_common_gtests",
            "android_vulkan_specific_gtests",
            "android_p6_isolated_scripts",
        ],
        mixins = [
            "gpu_pixel_6_experimental",
            "limited_capacity_bot",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.ANDROID,
    ),
    # Uncomment this entry when this experimental tester is actually in use.
    console_view_entry = consoles.console_view_entry(
        category = "test|android|arm64|rel|exp",
        short_name = "p6",
    ),
    list_view = "exp",
)

ci.thin_tester(
    name = "angle-android-arm64-google-pixel6-perf",
    description_html = "Perf tests release ANGLE on Android/arm64 on Pixel 6 devices",
    parent = "angle-android-arm64-builder-perf",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_android",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.ARM,
            target_bits = 64,
            target_platform = builder_config.target_platform.ANDROID,
        ),
        run_tests_serially = True,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "perf|android|arm64",
        short_name = "p6",
    ),
)

ci.thin_tester(
    name = "angle-android-arm64-google-pixel6-rel",
    description_html = "Tests release ANGLE on Android/arm64 on Pixel 6 devices",
    parent = "angle-android-arm64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_android",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.ARM,
            target_bits = 64,
            target_platform = builder_config.target_platform.ANDROID,
        ),
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [
            "android_common_gtests",
            "android_vulkan_specific_gtests",
            "android_p6_isolated_scripts",
        ],
        mixins = [
            "gpu_pixel_6_stable",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.ANDROID,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|android|arm64|rel",
        short_name = "p6",
    ),
)

ci.thin_tester(
    name = "angle-android-arm64-google-pixel10-rel",
    description_html = "Tests release ANGLE on Android/arm64 on Pixel 10 devices",
    parent = "angle-android-arm64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_android",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.ARM,
            target_bits = 64,
            target_platform = builder_config.target_platform.ANDROID,
        ),
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [
            "android_common_gtests",
            "android_gl_and_gles_gtests",
            "android_p10_gtests",
            "android_p10_isolated_scripts",
        ],
        mixins = [
            "gpu_pixel_10_stable",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.ANDROID,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|android|arm64|rel",
        short_name = "p10",
    ),
)

ci.thin_tester(
    name = "angle-android-arm64-samsung-s24-rel",
    description_html = "Tests release ANGLE on Android/arm64 on Samsung S24 devices",
    parent = "angle-android-arm64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_android",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.ARM,
            target_bits = 64,
            target_platform = builder_config.target_platform.ANDROID,
        ),
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [
            "android_common_gtests",
            "android_vulkan_specific_gtests",
            "android_s24_isolated_scripts",
        ],
        mixins = [
            # crbug.com/419062315
            "no_tombstones",
            "samsung_s24_stable",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.ANDROID,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|android|arm64|rel",
        short_name = "s24",
    ),
)

ci.thin_tester(
    name = "angle-linux-x64-amd-rx5500xt-rel",
    description_html = "Tests release ANGLE on Linux/x64 on AMD RX 5500 XT GPUs",
    parent = "angle-linux-x64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [],
        mixins = [
            "linux_amd_rx_5500_xt",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.LINUX,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|linux|x64|rel",
        short_name = "5500",
    ),
)

ci.thin_tester(
    name = "angle-linux-x64-intel-uhd630-exp-rel",
    description_html = "Tests release ANGLE on Linux/x64 on experimental Intel UHD 630 configs",
    parent = "angle-linux-x64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [],
        mixins = [
            "linux_intel_uhd_630_experimental",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.LINUX,
    ),
    # Uncomment this entry when this experimental tester is actually in use.
    # console_view_entry = consoles.console_view_entry(
    #     category = "test|linux|x64|rel|exp",
    #     short_name = "630",
    # ),
    list_view = "exp",
)

ci.thin_tester(
    name = "angle-linux-x64-intel-uhd630-perf",
    description_html = "Perf tests release ANGLE on Linux/x64 on Intel UHD 630 GPUs",
    parent = "angle-linux-x64-builder-perf",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
        run_tests_serially = True,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "perf|linux|x64",
        short_name = "630",
    ),
)

ci.thin_tester(
    name = "angle-linux-x64-intel-uhd630-rel",
    description_html = "Tests release ANGLE on Linux/x64 on Intel UHD 630 GPUs",
    parent = "angle-linux-x64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [
            "linux_real_hardware_common_gtests",
            "common_isolated_scripts",
        ],
        mixins = [
            "linux_intel_uhd_630_stable",
        ],
        per_test_modifications = {
            "angle_end2end_tests": targets.per_test_modification(
                replacements = targets.replacements(
                    args = {
                        # anglebug.com/408276172 suspecting WebGPU backend flakiness caused by
                        # multiprocess
                        "--max-processes": "1",
                    },
                ),
            ),
        },
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.LINUX,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|linux|x64|rel",
        short_name = "630",
    ),
)

ci.thin_tester(
    name = "angle-linux-x64-nvidia-gtx1660-exp-rel",
    description_html = "Tests release ANGLE on Linux/x64 on experimental NVIDIA GTX 1660 configs",
    parent = "angle-linux-x64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [],
        mixins = [
            "linux_nvidia_gtx_1660_experimental",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.LINUX,
    ),
    # Uncomment this entry when this experimental tester is actually in use.
    # console_view_entry = consoles.console_view_entry(
    #     category = "test|linux|x64|rel|exp",
    #     short_name = "1660",
    # ),
    list_view = "exp",
)

ci.thin_tester(
    name = "angle-linux-x64-nvidia-gtx1660-perf",
    description_html = "Perf tests release ANGLE on Linux/x64 on NVIDIA GTX 1660 GPUs",
    parent = "angle-linux-x64-builder-perf",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
        run_tests_serially = True,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "perf|linux|x64",
        short_name = "1660",
    ),
)

ci.thin_tester(
    name = "angle-linux-x64-nvidia-gtx1660-rel",
    description_html = "Tests release ANGLE on Linux/x64 on NVIDIA GTX 1660 GPUs",
    parent = "angle-linux-x64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [
            "linux_real_hardware_common_gtests",
            "linux_nvidia_only_gtests",
            "common_isolated_scripts",
        ],
        mixins = [
            "linux_nvidia_gtx_1660_stable",
        ],
        per_test_modifications = {
            "angle_deqp_egl_vulkan_tests": targets.remove(
                reason = "Occasionally hangs the machine http://anglebug.com/368553850",
            ),
        },
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.LINUX,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|linux|x64|rel",
        short_name = "1660",
    ),
)

ci.thin_tester(
    name = "angle-linux-x64-sws-asan",
    description_html = ("Tests release ANGLE on Linux/x64 with SwiftShader " +
                        "with ASan, LSan, and UBsan enabled"),
    parent = "angle-linux-x64-builder-asan",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
        run_tests_serially = True,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|linux|x64|asan",
        short_name = "sws",
    ),
)

ci.thin_tester(
    name = "angle-linux-x64-sws-rel",
    description_html = "Tests release ANGLE on Linux/x64 with SwiftShader",
    parent = "angle-linux-x64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [
            "swangle_gtests",
            "swangle_restricted_trace_gold_tests",
        ],
        mixins = [
            "gpu_linux_gce_stable",
            "timeout_15m",
        ],
        per_test_modifications = {
            "swangle_restricted_trace_gold_tests": targets.mixin(
                # anglebug.com/505781390 long time to download traces
                swarming = targets.swarming(
                    hard_timeout_sec = 1800,
                    io_timeout_sec = 1800,
                ),
            ),
        },
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.LINUX,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|linux|x64|rel",
        short_name = "sws",
    ),
)

ci.thin_tester(
    name = "angle-linux-x64-sws-tsan",
    description_html = "Tests release ANGLE on Linux/x64 with SwiftShader with TSan enabled",
    parent = "angle-linux-x64-builder-tsan",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
        run_tests_serially = True,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|linux|x64|tsan",
        short_name = "sws",
    ),
)

ci.thin_tester(
    name = "angle-mac-arm64-apple-m2-rel",
    description_html = "Tests release ANGLE on Mac/arm64 on Apple M2 SoCs",
    parent = "angle-mac-arm64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_nointernal",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.ARM,
            target_bits = 64,
            target_platform = builder_config.target_platform.MAC,
        ),
        use_test_trigger_cas = True,
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [
            "mac_arm64_gtests",
        ],
        mixins = [
            "mac_arm64_apple_m2_retina_gpu_stable",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.MAC,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|mac|arm64|rel",
        short_name = "m2",
    ),
)

ci.thin_tester(
    name = "angle-mac-x64-amd-5300m-exp-rel",
    description_html = "Tests release ANGLE on Mac/x64 on experimental configs of 16\" 2019 Macbook Pros w/ 5300M GPUs",
    parent = "angle-mac-x64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.MAC,
        ),
        use_test_trigger_cas = True,
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [],
        mixins = [
            "mac_retina_amd_gpu_experimental",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.MAC,
    ),
    # Uncomment this entry when this experimental tester is actually in use.
    # console_view_entry = consoles.console_view_entry(
    #     category = "test|mac|x64|rel|exp",
    #     short_name = "5300m",
    # ),
    list_view = "exp",
)

ci.thin_tester(
    name = "angle-mac-x64-amd-5300m-rel",
    description_html = "Tests release ANGLE on Mac/x64 on 16\" 2019 Macbook Pros w/ 5300M GPUs",
    parent = "angle-mac-x64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.MAC,
        ),
        use_test_trigger_cas = True,
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [
            "mac_x64_gtests",
        ],
        mixins = [
            "mac_retina_amd_gpu_stable",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.MAC,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|mac|x64|rel",
        short_name = "5300m",
    ),
)

ci.thin_tester(
    name = "angle-mac-x64-amd-555x-rel",
    description_html = "Tests release ANGLE on Mac/x64 on 15\" 2019 Macbook Pros w/ Radeon Pro 555X GPUs",
    parent = "angle-mac-x64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.MAC,
        ),
        use_test_trigger_cas = True,
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [
            "mac_x64_gtests",
        ],
        mixins = [
            "mac_retina_amd_555x_gpu_stable",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.MAC,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|mac|x64|rel",
        short_name = "555x",
    ),
)

ci.thin_tester(
    name = "angle-mac-x64-intel-uhd630-exp-rel",
    description_html = "Tests release ANGLE on Mac/x64 on experimental configs of 2018 Mac Minis w/ Intel UHD 630 GPUs",
    parent = "angle-mac-x64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.MAC,
        ),
        use_test_trigger_cas = True,
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [
            "mac_x64_gtests",
        ],
        mixins = [
            "mac_mini_intel_gpu_experimental",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.MAC,
    ),
    # Uncomment this entry when this experimental tester is actually in use.
    console_view_entry = consoles.console_view_entry(
        category = "test|mac|x64|rel|exp",
        short_name = "630",
    ),
    list_view = "exp",
)

ci.thin_tester(
    name = "angle-mac-x64-intel-uhd630-rel",
    description_html = "Tests release ANGLE on Mac/x64 on 2018 Mac Minis w/ Intel UHD 630 GPUs",
    parent = "angle-mac-x64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.MAC,
        ),
        use_test_trigger_cas = True,
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [
            "mac_x64_gtests",
        ],
        mixins = [
            "mac_mini_intel_gpu_stable",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.MAC,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|mac|x64|rel",
        short_name = "630",
    ),
)

ci.thin_tester(
    name = "angle-win-x64-intel-uhd630-exp-rel",
    description_html = "Tests release ANGLE on Win/x64 on experimental configs of Intel UHD 630 GPUs",
    parent = "angle-win-x64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_nointernal",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
        no_history = True,
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [],
        mixins = [
            "win10_intel_uhd_630_experimental",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.WINDOWS,
    ),
    # Uncomment this entry when this experimental tester is actually in use.
    # console_view_entry = consoles.console_view_entry(
    #     category = "test|win|x64|rel|exp",
    #     short_name = "630",
    # ),
    list_view = "exp",
)

ci.thin_tester(
    name = "angle-win-x64-intel-uhd630-perf",
    description_html = "Perf tests release ANGLE on Win/x64 on Intel UHD 630 GPUs",
    parent = "angle-win-x64-builder-perf",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
        run_tests_serially = True,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "perf|win|x64",
        short_name = "630",
    ),
)

ci.thin_tester(
    name = "angle-win-x64-intel-uhd630-rel",
    description_html = "Tests release ANGLE on Win/x64 on Intel UHD 630 GPUs",
    parent = "angle-win-x64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_nointernal",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
        no_history = True,
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [
            "common_isolated_scripts",
            "win_common_gtests",
        ],
        mixins = [
            "win10_intel_uhd_630_stable",
        ],
        per_test_modifications = {
            "angle_end2end_tests": targets.per_test_modification(
                mixins = targets.mixin(
                    args = [
                        # anglebug.com/40644897 suspecting device lost caused by
                        # multiprocess.
                        "--max-processes=1",
                    ],
                    swarming = targets.swarming(
                        shards = 10,
                    ),
                ),
                replacements = targets.replacements(
                    args = {
                        # crbug.com/506180945 low capacity in this pool.
                        "--gtest_filter": "-*Vulkan_SwiftShader*:*D3D9*:*WebGPU*",
                    },
                ),
            ),
            "angle_restricted_trace_gold_tests": targets.mixin(
                args = [
                    # anglebug.com/42263955 flaky 4x8 pixel artifacts on Win
                    # Intel.
                    "--flaky-retries=1",
                ],
                swarming = targets.swarming(
                    shards = 5,
                ),
            ),
            "angle_trace_perf_vulkan_smoke_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 3,
                ),
            ),
            "angle_white_box_tests": targets.mixin(
                args = [
                    # anglebug.com/40644897 suspecting device lost caused by
                    # multiprocess.
                    "--max-processes=1",
                ],
            ),
        },
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.WINDOWS,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|win|x64|rel",
        short_name = "630",
    ),
)

ci.thin_tester(
    name = "angle-win-x64-intel-uhd770-rel",
    description_html = "Tests release ANGLE on Win/x64 on Intel UHD 770 GPUs",
    parent = "angle-win-x64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_nointernal",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
        no_history = True,
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [
            "common_isolated_scripts",
            "win_common_gtests",
        ],
        mixins = [
            "win10_intel_uhd_770_stable",
        ],
        per_test_modifications = {
            "angle_deqp_gles2_d3d11_tests": targets.mixin(
                args = [
                    # anglebug.com/352528974 suspecting OOM caused by
                    # multiprocess.
                    "--max-processes=2",
                ],
            ),
            "angle_end2end_tests": targets.mixin(
                args = [
                    # anglebug.com/352528974 suspecting OOM caused by
                    # multiprocess.
                    "--max-processes=2",
                ],
                swarming = targets.swarming(
                    shards = 5,
                ),
            ),
            "angle_restricted_trace_gold_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 4,
                ),
            ),
            "angle_trace_perf_vulkan_smoke_tests": targets.mixin(
                swarming = targets.swarming(
                    shards = 4,
                ),
            ),
        },
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.WINDOWS,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|win|x64|rel",
        short_name = "770",
    ),
)

ci.thin_tester(
    name = "angle-win-x64-nvidia-gtx1660-exp-rel",
    description_html = "Tests release ANGLE on Win/x64 on experimental configs of NVIDIA GTX 1660 GPUs",
    parent = "angle-win-x64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_nointernal",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
        no_history = True,
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [],
        mixins = [
            "win10_nvidia_gtx_1660_experimental",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.WINDOWS,
    ),
    # Uncomment this entry when this experimental tester is actually in use.
    # console_view_entry = consoles.console_view_entry(
    #     category = "test|win|x64|rel|exp",
    #     short_name = "1660",
    # ),
    list_view = "exp",
)

ci.thin_tester(
    name = "angle-win-x64-nvidia-gtx1660-perf",
    description_html = "Perf tests release ANGLE on Win/x64 on NVIDIA GTX 1660 GPUs",
    parent = "angle-win-x64-builder-perf",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
        run_tests_serially = True,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "perf|win|x64",
        short_name = "1660",
    ),
)

ci.thin_tester(
    name = "angle-win-x64-nvidia-gtx1660-rel",
    description_html = "Tests release ANGLE on Win/x64 on NVIDIA GTX 1660 GPUs",
    parent = "angle-win-x64-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_nointernal",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
        no_history = True,
        run_tests_serially = True,
    ),
    targets = targets.bundle(
        targets = [
            "common_isolated_scripts",
            "win_common_gtests",
            "win_nvidia_only_gtests",
        ],
        mixins = [
            "win10_nvidia_gtx_1660_stable",
        ],
    ),
    targets_settings = targets.settings(
        browser_config = targets.browser_config.RELEASE,
        os_type = targets.os_type.WINDOWS,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|win|x64|rel",
        short_name = "1660",
    ),
)

ci.thin_tester(
    name = "angle-win-x64-sws-asan",
    description_html = "Tests release ANGLE on Win/x64 with ASan on SwiftShader",
    parent = "angle-win-x64-builder-asan",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_nointernal",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|win|x64|asan",
        short_name = "sws",
    ),
)

ci.thin_tester(
    name = "angle-win-x86-sws-rel",
    description_html = "Tests release ANGLE on Win/x86 with SwiftShader",
    parent = "angle-win-x86-builder-rel",
    builder_spec = builder_config.builder_spec(
        execution_mode = builder_config.execution_mode.TEST,
        gclient_config = builder_config.gclient_config(
            config = "angle_v2",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 32,
            target_platform = builder_config.target_platform.WIN,
        ),
        run_tests_serially = True,
    ),
    console_view_entry = consoles.console_view_entry(
        category = "test|win|x86|rel",
        short_name = "sws",
    ),
)

################################################################################
# Trace Tests                                                                  #
################################################################################

angle_linux_parent_builder(
    name = "angle-linux-x64-trace",
    description_html = "Runs ANGLE GLES trace tests on Linux/x64 with SwiftShader",
    schedule = "triggered",
    properties = {
        "run_trace_tests": True,
    },
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_nointernal",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.LINUX,
        ),
    ),
    # These GN args are not actually used since the trace tests do compilation
    # as part of running, but the recipe may try to "compile" as a side effect
    # of reusing the Chromium recipe code, so have some valid args.
    gn_args = gn_args.config(
        configs = [
            "linux_clang",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "trace|linux|x64",
        short_name = "rel",
    ),
)

angle_win_parent_builder(
    name = "angle-win-x64-trace",
    description_html = "Runs ANGLE GLES trace tests on Windows/x64 with SwiftShader",
    schedule = "triggered",
    properties = {
        "run_trace_tests": True,
    },
    builder_spec = builder_config.builder_spec(
        gclient_config = builder_config.gclient_config(
            config = "angle_v2_nointernal",
        ),
        chromium_config = builder_config.chromium_config(
            config = "angle_v2_clang",
            build_config = builder_config.build_config.RELEASE,
            target_arch = builder_config.target_arch.INTEL,
            target_bits = 64,
            target_platform = builder_config.target_platform.WIN,
        ),
    ),
    # These GN args are not actually used since the trace tests do compilation
    # as part of running, but the recipe may try to "compile" as a side effect
    # of reusing the Chromium recipe code, so have some valid args.
    gn_args = gn_args.config(
        configs = [
            "win_clang",
            "x64",
        ],
    ),
    console_view_entry = consoles.console_view_entry(
        category = "trace|win|x64",
        short_name = "rel",
    ),
)
