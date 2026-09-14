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
        revision = "3c067cfcf061330718cee8a4c74d9cd7232595c6",
    ),
)

pkg.depend(
    name = "@chromium-targets",
    source = pkg.source.googlesource(
        host = "chromium",
        repo = "chromium/src",
        ref = "refs/heads/main",
        path = "infra/config/targets",
        revision = "8d4c1b2a3b2e05b0c2dc6b4336ce75fb0f05552f",
    ),
)
