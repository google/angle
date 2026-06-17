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
