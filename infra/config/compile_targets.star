# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Compile target declarations

Compile targets can be referenced as additional_compile_targets in a bundle
declaration.
"""

load("@chromium-luci//targets.star", "targets")

targets.compile_target(
    name = "all",
    label = "//:all",
)

targets.compile_target(
    name = "angle",
    label = "//:angle",
)

targets.compile_target(
    name = "angle_shader_translator",
    label = "//:angle_shader_translator",
)

targets.compile_target(
    name = "angle_static",
    label = "//:angle_static",
)

targets.compile_target(
    name = "libfeature_support",
    label = "//:libfeature_support",
)

targets.compile_target(
    name = "libGLESv2_capture_complement",
    label = "//:libGLESv2_capture_complement",
)

targets.compile_target(
    name = "noop_target",
    label = "//:noop_target",
)

targets.compile_target(
    name = "translator_fuzzer",
    label = "//:translator_fuzzer",
)
