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
        revision = "b5db180b6ca4f82f62669c0acbbcc34655698d6a",
    ),
)

pkg.depend(
    name = "@chromium-targets",
    source = pkg.source.googlesource(
        host = "chromium",
        repo = "chromium/src",
        ref = "refs/heads/main",
        path = "infra/config/targets",
        revision = "bdd6350bab8e6fc35a80bf7a13a53237e2a6801b",
    ),
)
