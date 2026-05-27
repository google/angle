# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Binary declarations

Binaries can be referenced by tests and define the label of the compile target
to be built as well as various aspects that the infrastructure needs to know in
order to run the binary.
"""

load("@chromium-luci//targets.star", "targets")

targets.binaries.script(
    name = "angle_capture_tests",
    label = "//src/tests:angle_capture_tests",
    script = "//src/tests/capture_tests/capture_tests.py",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_egl_tests",
    label = "//src/tests:angle_deqp_egl_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles2_tests",
    label = "//src/tests:angle_deqp_gles2_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles3_tests",
    label = "//src/tests:angle_deqp_gles3_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles3_multisample_tests",
    label = "//src/tests:angle_deqp_gles3_multisample_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles3_rotate180_tests",
    label = "//src/tests:angle_deqp_gles3_rotate180_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles3_rotate270_tests",
    label = "//src/tests:angle_deqp_gles3_rotate270_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles3_rotate90_tests",
    label = "//src/tests:angle_deqp_gles3_rotate90_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles31_multisample_tests",
    label = "//src/tests:angle_deqp_gles31_multisample_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles31_rotate180_tests",
    label = "//src/tests:angle_deqp_gles31_rotate180_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles31_rotate270_tests",
    label = "//src/tests:angle_deqp_gles31_rotate270_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles31_rotate90_tests",
    label = "//src/tests:angle_deqp_gles31_rotate90_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_gles31_tests",
    label = "//src/tests:angle_deqp_gles31_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_khr_gles2_tests",
    label = "//src/tests:angle_deqp_khr_gles2_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_khr_gles31_tests",
    label = "//src/tests:angle_deqp_khr_gles31_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_khr_gles32_tests",
    label = "//src/tests:angle_deqp_khr_gles32_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_khr_gles3_tests",
    label = "//src/tests:angle_deqp_khr_gles3_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_khr_glesext_tests",
    label = "//src/tests:angle_deqp_khr_glesext_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_khr_noctx_gles2_tests",
    label = "//src/tests:angle_deqp_khr_noctx_gles2_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_khr_noctx_gles32_tests",
    label = "//src/tests:angle_deqp_khr_noctx_gles32_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_deqp_khr_single_gles32_tests",
    label = "//src/tests:angle_deqp_khr_single_gles32_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_end2end_tests",
    label = "//src/tests:angle_end2end_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_gles1_conformance_tests",
    label = "//src/tests:angle_gles1_conformance_tests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_oclcts_api",
    label = "//src/tests:angle_oclcts_api",
)

targets.binaries.windowed_test_launcher(
    name = "angle_oclcts_basic",
    label = "//src/tests:angle_oclcts_basic",
)

targets.binaries.windowed_test_launcher(
    name = "angle_oclcts_bruteforce",
    label = "//src/tests:angle_oclcts_bruteforce",
)

targets.binaries.windowed_test_launcher(
    name = "angle_oclcts_buffers",
    label = "//src/tests:angle_oclcts_buffers",
)

targets.binaries.windowed_test_launcher(
    name = "angle_oclcts_cl_copy_images",
    label = "//src/tests:angle_oclcts_cl_copy_images",
)

targets.binaries.windowed_test_launcher(
    name = "angle_oclcts_cl_fill_images",
    label = "//src/tests:angle_oclcts_cl_fill_images",
)

targets.binaries.windowed_test_launcher(
    name = "angle_oclcts_cl_get_info",
    label = "//src/tests:angle_oclcts_cl_get_info",
)

targets.binaries.windowed_test_launcher(
    name = "angle_oclcts_compiler",
    label = "//src/tests:angle_oclcts_compiler",
)

targets.binaries.windowed_test_launcher(
    name = "angle_oclcts_events",
    label = "//src/tests:angle_oclcts_events",
)

targets.binaries.windowed_test_launcher(
    name = "angle_oclcts_multiples",
    label = "//src/tests:angle_oclcts_multiples",
)

targets.binaries.windowed_test_launcher(
    name = "angle_oclcts_non_uniform_work_group",
    label = "//src/tests:angle_oclcts_non_uniform_work_group",
)

targets.binaries.windowed_test_launcher(
    name = "angle_oclcts_profiling",
    label = "//src/tests:angle_oclcts_profiling",
)

targets.binaries.script(
    name = "angle_perftests",
    label = "//src/tests:angle_perftests",
    script = "//src/tests/run_perf_tests.py",
)

targets.binaries.script(
    name = "angle_restricted_trace_gold_tests",
    label = "//src/tests/restricted_traces:angle_restricted_trace_gold_tests",
    script = "//src/tests/restricted_traces/restricted_trace_gold_tests.py",
)

targets.binaries.script(
    name = "angle_trace_perf_tests",
    label = "//src/tests:angle_trace_perf_tests",
    script = "//src/tests/run_perf_tests.py",
)

targets.binaries.windowed_test_launcher(
    name = "angle_unittests",
    label = "//src/tests:angle_unittests",
)

targets.binaries.windowed_test_launcher(
    name = "angle_white_box_tests",
    label = "//src/tests:angle_white_box_tests",
)
