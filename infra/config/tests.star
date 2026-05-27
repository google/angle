# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Test declarations

Tests define a target to be built and executed on a builder. Tests can
be referenced by a suite or bundle to include the test in the
suite/bundle. Tests also define a bundle containing just the test
itself, so they can be used wherever a bundle is expected.
"""

load("@chromium-luci//targets.star", "targets")

targets.tests.isolated_script_test(
    name = "angle_capture_tests",
    mixins = [
        "isolated_script_log_debug",
    ],
    binary = "angle_capture_tests",
)

targets.tests.gtest_test(
    name = "angle_cl_api_tests",
    mixins = [
        "no_xvfb",
        "use_isolated_scripts_api",
    ],
    binary = "angle_oclcts_api",
)

targets.tests.gtest_test(
    name = "angle_cl_basic_tests",
    mixins = [
        "no_xvfb",
        "use_isolated_scripts_api",
    ],
    binary = "angle_oclcts_basic",
)

targets.tests.gtest_test(
    name = "angle_cl_bruteforce_tests",
    mixins = [
        "no_xvfb",
        "use_isolated_scripts_api",
    ],
    binary = "angle_oclcts_bruteforce",
)

targets.tests.gtest_test(
    name = "angle_cl_buffers_tests",
    mixins = [
        "no_xvfb",
        "use_isolated_scripts_api",
    ],
    binary = "angle_oclcts_buffers",
)

targets.tests.gtest_test(
    name = "angle_cl_compiler_tests",
    mixins = [
        "no_xvfb",
        "use_isolated_scripts_api",
    ],
    binary = "angle_oclcts_compiler",
)

targets.tests.gtest_test(
    name = "angle_cl_copy_images_tests",
    mixins = [
        "no_xvfb",
        "use_isolated_scripts_api",
    ],
    binary = "angle_oclcts_cl_copy_images",
)

targets.tests.gtest_test(
    name = "angle_cl_events_tests",
    mixins = [
        "no_xvfb",
        "use_isolated_scripts_api",
    ],
    binary = "angle_oclcts_events",
)

targets.tests.gtest_test(
    name = "angle_cl_fill_images_tests",
    mixins = [
        "no_xvfb",
        "use_isolated_scripts_api",
    ],
    binary = "angle_oclcts_cl_fill_images",
)

targets.tests.gtest_test(
    name = "angle_cl_images_get_info_tests",
    mixins = [
        "no_xvfb",
        "use_isolated_scripts_api",
    ],
    binary = "angle_oclcts_cl_get_info",
)

targets.tests.gtest_test(
    name = "angle_cl_multiples_tests",
    mixins = [
        "no_xvfb",
        "use_isolated_scripts_api",
    ],
    binary = "angle_oclcts_multiples",
)

targets.tests.gtest_test(
    name = "angle_cl_non_uniform_work_group_tests",
    mixins = [
        "no_xvfb",
        "use_isolated_scripts_api",
    ],
    binary = "angle_oclcts_non_uniform_work_group",
)

targets.tests.gtest_test(
    name = "angle_cl_profiling_tests",
    mixins = [
        "no_xvfb",
        "use_isolated_scripts_api",
    ],
    binary = "angle_oclcts_profiling",
)

targets.tests.gtest_test(
    name = "angle_deqp_egl_gl_tests",
    mixins = [
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_gl",
        "use_isolated_scripts_api",
    ],
    args = [
        # Flaky when run with multiple processes.
        "--max-processes=1",
    ],
    binary = "angle_deqp_egl_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_egl_vulkan_tests",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
        targets.mixin(
            android_swarming = targets.swarming(
                shards = 4,
            ),
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    ],
    binary = "angle_deqp_egl_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_gles2_gl_tests",
    mixins = [
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_gl",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_gles2_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_gles2_vulkan_tests",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
        targets.mixin(
            android_swarming = targets.swarming(
                shards = 4,
            ),
        ),
    ],
    binary = "angle_deqp_gles2_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_gles2_webgpu_tests",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_webgpu",
        "use_isolated_scripts_api",
        targets.mixin(
            android_swarming = targets.swarming(
                shards = 4,
            ),
        ),
    ],
    binary = "angle_deqp_gles2_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_gles31_gl_tests",
    mixins = [
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_gl",
        "use_isolated_scripts_api",
        targets.mixin(
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    ],
    binary = "angle_deqp_gles31_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_gles31_multisample_vulkan_tests",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_gles31_multisample_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_gles31_vulkan_rotate180_tests",
    mixins = [
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_gles31_rotate180_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_gles31_vulkan_rotate270_tests",
    mixins = [
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_gles31_rotate270_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_gles31_vulkan_rotate90_tests",
    mixins = [
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_gles31_rotate90_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_gles31_vulkan_tests",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
        targets.mixin(
            android_swarming = targets.swarming(
                shards = 20,
            ),
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    ],
    binary = "angle_deqp_gles31_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_gles3_gl_tests",
    mixins = [
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_gl",
        "use_isolated_scripts_api",
        targets.mixin(
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    ],
    binary = "angle_deqp_gles3_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_gles3_multisample_vulkan_tests",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_gles3_multisample_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_gles3_vulkan_rotate180_tests",
    mixins = [
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
        targets.mixin(
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    ],
    binary = "angle_deqp_gles3_rotate180_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_gles3_vulkan_rotate270_tests",
    mixins = [
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
        targets.mixin(
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    ],
    binary = "angle_deqp_gles3_rotate270_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_gles3_vulkan_rotate90_tests",
    mixins = [
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
        targets.mixin(
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    ],
    binary = "angle_deqp_gles3_rotate90_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_gles3_vulkan_tests",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
        targets.mixin(
            android_swarming = targets.swarming(
                shards = 12,
            ),
            swarming = targets.swarming(
                shards = 4,
            ),
        ),
    ],
    binary = "angle_deqp_gles3_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_khr_gles2_vulkan_tests",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_khr_default_height_width_seed",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_khr_gles2_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_khr_gles31_vulkan_tests",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_khr_default_height_width_seed",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_khr_gles31_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_khr_gles32_vulkan_tests_seed1_width64_height64",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_khr_default_height_width_seed",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_khr_gles32_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_khr_gles32_vulkan_tests_seed2_width113_height47",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
    ],
    args = [
        "--deqp-base-seed=2",
        "--surface-width=113",
        "--surface-height=47",
    ],
    binary = "angle_deqp_khr_gles32_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_khr_gles32_vulkan_tests_seed3_width-1_height64",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
    ],
    args = [
        "--deqp-base-seed=3",
        "--surface-width=-1",
        "--surface-height=64",
        "--deqp-gl-config-name=rgba8888d24s8",
    ],
    binary = "angle_deqp_khr_gles32_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_khr_gles32_vulkan_tests_seed3_width64_height-1",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
    ],
    args = [
        "--deqp-base-seed=3",
        "--surface-width=64",
        "--surface-height=-1",
        "--deqp-gl-config-name=rgba8888d24s8",
    ],
    binary = "angle_deqp_khr_gles32_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_khr_gles3_vulkan_tests",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_khr_default_height_width_seed",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_khr_gles3_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_khr_glesext_vulkan_tests",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_khr_default_height_width_seed",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_khr_glesext_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_khr_noctx_gles2_vulkan_tests",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_khr_noctx_gles2_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_khr_noctx_gles32_vulkan_tests",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_khr_noctx_gles32_tests",
)

targets.tests.gtest_test(
    name = "angle_deqp_khr_single_gles32_vulkan_tests",
    mixins = [
        "android_deqp_increased_verbosity_and_shard_timeout",
        "deqp_merge_script",
        "no_xvfb",
        "use_angle_vulkan",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_khr_single_gles32_tests",
)

targets.tests.gtest_test(
    name = "angle_end2end_tests",
    mixins = [
        "angle_end2end_tests_common_args",
        "gtest_filter_no_vulkan_swiftshader",
        "no_xvfb",
        "use_isolated_scripts_api",
        targets.mixin(
            android_swarming = targets.swarming(
                shards = 8,
            ),
            swarming = targets.swarming(
                shards = 3,
            ),
        ),
    ],
    binary = "angle_end2end_tests",
)

targets.tests.gtest_test(
    name = "angle_gles1_conformance_tests",
    mixins = [
        "no_xvfb",
        "use_isolated_scripts_api",
        targets.mixin(
            android_args = [
                "--shard-timeout=600",
                "-v",
            ],
        ),
    ],
    args = [
        "--test-timeout=300",
        "--batch-size=10",
    ],
    binary = "angle_gles1_conformance_tests",
)

targets.tests.isolated_script_test(
    name = "angle_trace_perf_native_smoke_tests",
    mixins = [
        "isolated_script_log_debug",
        "perf_merge_script_smoke_test_mode",
        "perf_tests_smoke_test_mode",
        targets.mixin(
            android_swarming = targets.swarming(
                shards = 6,
            ),
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    ],
    args = [
        "--use-gl=native",
        "--trace-tests",
    ],
    binary = "angle_trace_perf_tests",
)

targets.tests.isolated_script_test(
    name = "angle_trace_perf_vulkan_smoke_tests",
    mixins = [
        "isolated_script_log_debug",
        "perf_merge_script_smoke_test_mode",
        "perf_tests_smoke_test_mode",
        "use_angle_vulkan",
        targets.mixin(
            android_swarming = targets.swarming(
                shards = 6,
            ),
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    ],
    args = [
        "--trace-tests",
    ],
    binary = "angle_trace_perf_tests",
)

targets.tests.isolated_script_test(
    name = "angle_restricted_trace_gold_tests",
    mixins = [
        "angle_skia_gold_test",
        targets.mixin(
            android_args = [
                "-v",
            ],
        ),
        targets.mixin(
            android_swarming = targets.swarming(
                shards = 6,
            ),
            swarming = targets.swarming(
                shards = 3,
            ),
        ),
    ],
    args = [
        "--test-machine-name",
        "${buildername}",
    ],
    binary = "angle_restricted_trace_gold_tests",
)

targets.tests.isolated_script_test(
    name = "angle_smoke_perftests",
    mixins = [
        "isolated_script_log_debug",
        "perf_merge_script_smoke_test_mode",
        "perf_tests_smoke_test_mode",
        targets.mixin(
            android_swarming = targets.swarming(
                shards = 6,
            ),
            swarming = targets.swarming(
                shards = 2,
            ),
        ),
    ],
    binary = "angle_perftests",
)

targets.tests.gtest_test(
    name = "angle_unittests",
    mixins = [
        "no_xvfb",
        "use_isolated_scripts_api",
        targets.mixin(
            android_args = [
                "-v",
            ],
        ),
    ],
    binary = "angle_unittests",
)

targets.tests.gtest_test(
    name = "angle_white_box_tests",
    mixins = [
        "no_xvfb",
        "use_isolated_scripts_api",
        targets.mixin(
            android_args = [
                "--shard-timeout=180",
                "-v",
            ],
        ),
    ],
    binary = "angle_white_box_tests",
)

targets.tests.gtest_test(
    name = "swangle_deqp_egl_tests",
    mixins = [
        "use_angle_swiftshader",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_egl_tests",
)

targets.tests.gtest_test(
    name = "swangle_deqp_gles2_tests",
    mixins = [
        "use_angle_swiftshader",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_gles2_tests",
)

targets.tests.gtest_test(
    name = "swangle_deqp_gles31_multisample_tests",
    mixins = [
        "use_angle_swiftshader",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_gles31_multisample_tests",
)

targets.tests.gtest_test(
    name = "swangle_deqp_gles31_rotate180_tests",
    mixins = [
        "use_angle_swiftshader",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_gles31_rotate180_tests",
)

targets.tests.gtest_test(
    name = "swangle_deqp_gles31_rotate270_tests",
    mixins = [
        "use_angle_swiftshader",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_gles31_rotate270_tests",
)

targets.tests.gtest_test(
    name = "swangle_deqp_gles31_rotate90_tests",
    mixins = [
        "use_angle_swiftshader",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_gles31_rotate90_tests",
)

targets.tests.gtest_test(
    name = "swangle_deqp_gles31_tests",
    mixins = [
        "use_angle_swiftshader",
        "use_isolated_scripts_api",
        targets.mixin(
            swarming = targets.swarming(
                shards = 10,
            ),
        ),
    ],
    binary = "angle_deqp_gles31_tests",
)

targets.tests.gtest_test(
    name = "swangle_deqp_gles3_multisample_tests",
    mixins = [
        "use_angle_swiftshader",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_gles3_multisample_tests",
)

targets.tests.gtest_test(
    name = "swangle_deqp_gles3_rotate180_tests",
    mixins = [
        "use_angle_swiftshader",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_gles3_rotate180_tests",
)

targets.tests.gtest_test(
    name = "swangle_deqp_gles3_rotate270_tests",
    mixins = [
        "use_angle_swiftshader",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_gles3_rotate270_tests",
)

targets.tests.gtest_test(
    name = "swangle_deqp_gles3_rotate90_tests",
    mixins = [
        "use_angle_swiftshader",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_gles3_rotate90_tests",
)

targets.tests.gtest_test(
    name = "swangle_deqp_gles3_tests",
    mixins = [
        "use_angle_swiftshader",
        "use_isolated_scripts_api",
        targets.mixin(
            swarming = targets.swarming(
                shards = 4,
            ),
        ),
    ],
    binary = "angle_deqp_gles3_tests",
)

targets.tests.gtest_test(
    name = "swangle_deqp_khr_gles2_tests",
    mixins = [
        "deqp_khr_default_height_width_seed",
        "use_angle_swiftshader",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_khr_gles2_tests",
)

targets.tests.gtest_test(
    name = "swangle_deqp_khr_gles31_tests",
    mixins = [
        "deqp_khr_default_height_width_seed",
        "use_angle_swiftshader",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_khr_gles31_tests",
)

targets.tests.gtest_test(
    name = "swangle_deqp_khr_gles3_tests",
    mixins = [
        "deqp_khr_default_height_width_seed",
        "use_angle_swiftshader",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_khr_gles3_tests",
)

targets.tests.gtest_test(
    name = "swangle_deqp_khr_noctx_gles2_tests",
    mixins = [
        "use_angle_swiftshader",
        "use_isolated_scripts_api",
    ],
    binary = "angle_deqp_khr_noctx_gles2_tests",
)

targets.tests.gtest_test(
    name = "swangle_end2end_tests",
    mixins = [
        "gtest_filter_only_vulkan_swiftshader",
        "use_isolated_scripts_api",
    ],
    binary = "angle_end2end_tests",
)

targets.tests.isolated_script_test(
    name = "swangle_restricted_trace_gold_tests",
    mixins = [
        "angle_skia_gold_test",
        targets.mixin(
            linux_args = [
                "--xvfb",
                # An X Windows bug sometimes causes flaky connection errors.
                "--flaky-retries=2",
            ],
        ),
        targets.mixin(
            swarming = targets.swarming(
                shards = 8,
            ),
        ),
    ],
    args = [
        "--test-machine-name",
        "${buildername}",
        "--swiftshader",
        "--key-frame-limit=10",
    ],
    binary = "angle_restricted_trace_gold_tests",
)

targets.tests.gtest_test(
    name = "swangle_white_box_tests",
    mixins = [
        "gtest_filter_only_vulkan_swiftshader",
        "use_isolated_scripts_api",
    ],
    binary = "angle_white_box_tests",
)
