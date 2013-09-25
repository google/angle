// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "GLSLANG/ShaderLang.h"

int main(int argc, char** argv) {
  testing::InitGoogleMock(&argc, argv);
  ShInitialize();
  int rt = RUN_ALL_TESTS();
  ShFinalize();
  return rt;
}
