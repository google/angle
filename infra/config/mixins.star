# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Mixin declarations

Mixins are used to define common properties that can be applied to multiple
tests.

These are the ANGLE-specific mixins. Mixins shared with Chromium (e.g. for
Swarming dimensions) are pulled in via the @chromium-targets Starlark package
and are found at Chromium's //infra/config/targets/mixins.star file.
"""

load("@chromium-luci//targets.star", "targets")

targets.mixin(
    name = "android_deqp_increased_verbosity_and_shard_timeout",
    android_args = [
        "-v",
        "--shard-timeout=500",
    ],
)

targets.mixin(
    name = "angle_end2end_tests_common_args",
    android_args = [
        "--shard-timeout=180",
        "-v",
        # We use this argument to save test artifacts.
        "--render-test-output-dir=${ISOLATED_OUTDIR}",
    ],
    linux_args = [
        # Linux has issues with creating too many windows at once.
        "--max-processes=4",
    ],
)

targets.mixin(
    name = "angle_skia_gold_test",
    args = [
        "--git-revision=${got_angle_revision}",
        # BREAK GLASS IN CASE OF EMERGENCY
        # Uncommenting this argument will bypass all interactions with Skia
        # Gold in any tests that use it. This is meant as a temporary
        # emergency stop in case of a Gold outage that's affecting the bots.
        # "--bypass-skia-gold-functionality",
    ],
    precommit_args = [
        "--gerrit-issue=${patch_issue}",
        "--gerrit-patchset=${patch_set}",
        "--buildbucket-id=${buildbucket_build_id}",
        # This normally evaluates to "0", but will evaluate to "1" if
        # "Use-Permissive-Angle-Pixel-Comparison: True" is present as a
        # CL footer.
        "--use-permissive-pixel-comparison=${use_permissive_angle_pixel_comparison}",
    ],
)

targets.mixin(
    name = "deqp_khr_default_height_width_seed",
    args = [
        "--surface-height=64",
        "--surface-width=64",
        "--deqp-base-seed=1",
    ],
)

targets.mixin(
    name = "deqp_merge_script",
    merge = targets.merge(
        script = "//scripts/angle_deqp_test_merge.py",
    ),
)

targets.mixin(
    name = "enable_trace_tests",
    args = [
        "--trace-tests",
    ],
)

targets.mixin(
    name = "gtest_enable_flaky_retries",
    args = [
        # Meant for working around flaky crashes. http://anglebug.com/42265067
        "--flaky-retries=2",
    ],
)

targets.mixin(
    name = "gtest_filter_no_vulkan",
    args = [
        "--gtest_filter=-*Vulkan*",
    ],
)

targets.mixin(
    name = "gtest_filter_no_vulkan_swiftshader",
    args = [
        "--gtest_filter=-*Vulkan_SwiftShader*",
    ],
)

targets.mixin(
    name = "gtest_filter_only_real_vulkan",
    args = [
        "--gtest_filter=*Vulkan*:-*Vulkan_SwiftShader*",
    ],
)

targets.mixin(
    name = "gtest_filter_only_vulkan",
    args = [
        "--gtest_filter=*Vulkan*",
    ],
)

targets.mixin(
    name = "gtest_filter_only_vulkan_swiftshader",
    args = [
        "--gtest_filter=*Vulkan_SwiftShader*",
    ],
)

targets.mixin(
    name = "isolated_script_log_debug",
    args = [
        "--log=debug",
    ],
)

targets.mixin(
    name = "no_xvfb",
    linux_args = [
        "--no-xvfb",
    ],
)

targets.mixin(
    name = "perf_merge_script",
    merge = targets.merge(
        script = "//scripts/process_angle_perf_results.py",
    ),
)

targets.mixin(
    name = "perf_merge_script_smoke_test_mode",
    merge = targets.merge(
        script = "//scripts/process_angle_perf_results.py",
        args = [
            "--smoke-test-mode",
        ],
    ),
)

targets.mixin(
    name = "perf_tests_common_args",
    android_args = [
        "--trial-time=10",
    ],
    args = [
        "--samples-per-test=3",
        "--trials-per-sample=3",
        "--show-test-stdout",
    ],
)

targets.mixin(
    name = "perf_tests_sharding",
    android_swarming = targets.swarming(
        shards = 30,
    ),
    swarming = targets.swarming(
        shards = 10,
    ),
)

targets.mixin(
    name = "perf_tests_smoke_test_mode",
    args = [
        "--smoke-test-mode",
        "--show-test-stdout",
    ],
)

targets.mixin(
    name = "samsung_s24_stable",
    swarming = targets.swarming(
        dimensions = {
            "device_os": "AP3A.240905.015.A2",
            "device_os_type": "user",
            "device_type": "s5e9945",
            "os": "Android",
            "pool": "chromium.tests.gpu",
        },
    ),
)

targets.mixin(
    name = "timeout_120m",
    swarming = targets.swarming(
        hard_timeout_sec = 7200,
        io_timeout_sec = 300,
    ),
)

targets.mixin(
    name = "use_angle_d3d11",
    args = [
        "--use-angle=d3d11",
    ],
)

targets.mixin(
    name = "use_angle_gl",
    args = [
        "--use-angle=gl",
    ],
)

targets.mixin(
    name = "use_angle_gles",
    args = [
        "--use-angle=gles",
    ],
)

targets.mixin(
    name = "use_angle_metal",
    args = [
        "--use-angle=metal",
    ],
)

targets.mixin(
    name = "use_angle_swiftshader",
    args = [
        "--use-angle=swiftshader",
    ],
)

targets.mixin(
    name = "use_angle_vulkan",
    args = [
        "--use-angle=vulkan",
    ],
)

targets.mixin(
    name = "use_angle_webgpu",
    args = [
        "--use-angle=webgpu",
    ],
)

targets.mixin(
    name = "use_gl_native",
    args = [
        "--use-gl=native",
    ],
)

targets.mixin(
    name = "use_isolated_scripts_api",
    use_isolated_scripts_api = True,
)
