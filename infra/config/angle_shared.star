# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Code shared by both CI and try builders."""

def _apply_win_msvc_builder_defaults(kwargs):
    # n2-standard-8 is specifically targeted for Win/MSVC instead of the more
    # common e2-standard-8 because Windows compilation takes the most time and
    # the use of MSVC means that RBE is unsupported for remote compilation. The
    # newer CPUs used by n2-standard-8 GCE instances result in significantly
    # faster compile times. e4-standard-8 will be the standard going forward for
    # all builders and appears to have equivalent MSVC build times to
    # n2-standard-8, so allow that as well until all n2-standard-8 machines are
    # phased out.
    kwargs.setdefault("machine_type", "n2-standard-8|e4-standard-8")
    return kwargs

builder_defaults = struct(
    apply_win_msvc_builder_defaults = _apply_win_msvc_builder_defaults,
)
