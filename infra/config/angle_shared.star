# Copyright 2026 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Code shared by both CI and try builders."""

load("@chromium-luci//builders.star", "os")

def _apply_linux_builder_defaults(kwargs):
    kwargs.setdefault("cores", 8)
    kwargs.setdefault("os", os.LINUX_DEFAULT)
    kwargs.setdefault("ssd", None)
    return kwargs

def _apply_mac_builder_defaults(kwargs):
    kwargs.setdefault("cpu", "arm64")
    kwargs.setdefault("os", os.MAC_DEFAULT)
    return kwargs

def _apply_win_clang_builder_defaults(kwargs):
    kwargs.setdefault("cores", 8)
    kwargs.setdefault("os", os.WINDOWS_DEFAULT)
    kwargs.setdefault("ssd", None)
    return kwargs

def _apply_win_msvc_builder_defaults(kwargs):
    kwargs.setdefault("cores", 8)
    kwargs.setdefault("os", os.WINDOWS_DEFAULT)
    kwargs.setdefault("ssd", None)
    kwargs.setdefault("machine_type", "n2-standard-8")
    return kwargs

builder_defaults = struct(
    apply_linux_builder_defaults = _apply_linux_builder_defaults,
    apply_mac_builder_defaults = _apply_mac_builder_defaults,
    apply_win_clang_builder_defaults = _apply_win_clang_builder_defaults,
    apply_win_msvc_builder_defaults = _apply_win_msvc_builder_defaults,
)
