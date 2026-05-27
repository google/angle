# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Bundle declarations

Bundles are groupings of tests and/or compile targets that can be referenced by
builders or other bundles.
"""

load("@chromium-luci//targets.star", "targets")

targets.bundle(
    name = "linux_real_hardware_common_gtests",
    targets = [
        "angle_deqp_gles2_gl_tests",
        "angle_end2end_tests",
        "angle_gles1_conformance_tests",
        "angle_unittests",
        "angle_white_box_tests",
    ],
)

targets.bundle(
    name = "linux_nvidia_only_gtests",
    targets = [
        "angle_deqp_egl_gl_tests",
        "angle_deqp_egl_vulkan_tests",
        "angle_deqp_gles2_vulkan_tests",
        "angle_deqp_gles2_webgpu_tests",
        "angle_deqp_gles31_gl_tests",
        "angle_deqp_gles31_multisample_vulkan_tests",
        "angle_deqp_gles31_vulkan_tests",
        "angle_deqp_gles31_vulkan_rotate180_tests",
        "angle_deqp_gles31_vulkan_rotate270_tests",
        "angle_deqp_gles31_vulkan_rotate90_tests",
        "angle_deqp_gles3_gl_tests",
        "angle_deqp_gles3_multisample_vulkan_tests",
        "angle_deqp_gles3_vulkan_tests",
        "angle_deqp_gles3_vulkan_rotate180_tests",
        "angle_deqp_gles3_vulkan_rotate270_tests",
        "angle_deqp_gles3_vulkan_rotate90_tests",
        "angle_deqp_khr_gles2_vulkan_tests",
        "angle_deqp_khr_gles31_vulkan_tests",
        "angle_deqp_khr_gles32_vulkan_gtests",
        "angle_deqp_khr_gles3_vulkan_tests",
        "angle_deqp_khr_glesext_vulkan_tests",
        "angle_deqp_khr_noctx_gles2_vulkan_tests",
        "angle_deqp_khr_noctx_gles32_vulkan_tests",
        "angle_deqp_khr_single_gles32_vulkan_tests",
        "angle_cl_api_tests",
        "angle_cl_basic_tests",
        "angle_cl_bruteforce_tests",
        "angle_cl_buffers_tests",
        "angle_cl_compiler_tests",
        "angle_cl_copy_images_tests",
        "angle_cl_events_tests",
        "angle_cl_fill_images_tests",
        "angle_cl_images_get_info_tests",
        "angle_cl_multiples_tests",
        "angle_cl_non_uniform_work_group_tests",
        "angle_cl_profiling_tests",
    ],
)

targets.bundle(
    name = "angle_deqp_khr_gles32_vulkan_gtests",
    targets = [
        "angle_deqp_khr_gles32_vulkan_tests_seed1_width64_height64",
        "angle_deqp_khr_gles32_vulkan_tests_seed2_width113_height47",
        "angle_deqp_khr_gles32_vulkan_tests_seed3_width-1_height64",
        "angle_deqp_khr_gles32_vulkan_tests_seed3_width64_height-1",
    ],
)

targets.bundle(
    name = "common_isolated_scripts",
    targets = [
        "angle_capture_tests",
        "angle_smoke_perftests",
        "angle_restricted_trace_gold_tests",
        "angle_trace_perf_native_smoke_tests",
        "angle_trace_perf_vulkan_smoke_tests",
    ],
)

targets.bundle(
    name = "swangle_gtests",
    targets = [
        "swangle_deqp_egl_tests",
        "swangle_deqp_gles2_tests",
        "swangle_deqp_gles31_multisample_tests",
        "swangle_deqp_gles31_rotate180_tests",
        "swangle_deqp_gles31_rotate270_tests",
        "swangle_deqp_gles31_rotate90_tests",
        "swangle_deqp_gles31_tests",
        "swangle_deqp_gles3_multisample_tests",
        "swangle_deqp_gles3_rotate180_tests",
        "swangle_deqp_gles3_rotate270_tests",
        "swangle_deqp_gles3_rotate90_tests",
        "swangle_deqp_gles3_tests",
        "swangle_deqp_khr_gles2_tests",
        "swangle_deqp_khr_gles31_tests",
        "swangle_deqp_khr_gles3_tests",
        "swangle_deqp_khr_noctx_gles2_tests",
        "swangle_end2end_tests",
        "swangle_white_box_tests",
    ],
)
