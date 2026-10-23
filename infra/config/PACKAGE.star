#!/usr/bin/env lucicfg
#
# Copyright 2025 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Package declaration for the Angle project."""

pkg.declare(
    name = "@angle-project",
    lucicfg = "1.45.8",
)

pkg.options.lint_checks([
    "all",
])

pkg.entrypoint("main.star")

pkg.depend(
    name = "@chromium-luci",
    source = pkg.source.googlesource(
        host = "chromium",
        repo = "infra/chromium",
        ref = "refs/heads/main",
        path = "starlark-libs/chromium-luci",
        revision = "352a5c647413dd05dc3ff8b28df99ae8d5327acb",
    ),
)

pkg.depend(
    name = "@chromium-targets",
    source = pkg.source.googlesource(
        host = "chromium",
        repo = "chromium/src",
        ref = "refs/heads/main",
        path = "infra/config/targets",
        revision = "d9b632d973ec9fa5175d2124ba0d5bb52d80aabf",
    ),
)
