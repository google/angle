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
        revision = "8c0ad4ba2dc0a7f10a34376a631159162a823eeb",
    ),
)

pkg.depend(
    name = "@chromium-targets",
    source = pkg.source.googlesource(
        host = "chromium",
        repo = "chromium/src",
        ref = "refs/heads/main",
        path = "infra/config/targets",
        revision = "82de21bc9adaeb2e3cb4f4c987c0ca04962645b4",
    ),
)
