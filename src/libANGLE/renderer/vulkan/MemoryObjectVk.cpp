// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// MemoryObjectVk.cpp: Defines the class interface for MemoryObjectVk, implementing
// MemoryObjectImpl.

#include "libANGLE/renderer/vulkan/MemoryObjectVk.h"

namespace rx
{

MemoryObjectVk::MemoryObjectVk() {}

MemoryObjectVk::~MemoryObjectVk() = default;

void MemoryObjectVk::onDestroy(const gl::Context *context) {}

}  // namespace rx
