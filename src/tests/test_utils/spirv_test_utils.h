//
// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef TESTS_TEST_UTILS_SPIRV_TEST_UTILS_H_
#define TESTS_TEST_UTILS_SPIRV_TEST_UTILS_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <spirv/unified1/spirv.hpp>

namespace angle
{
// Extract SPIR-V from ShaderVk's debug output before program-specific transformations.
// Checks encoding and instruction syntax only. Failure clears binary.
testing::AssertionResult GetSpirvFromDebugInfo(const std::string &debugInfo,
                                               std::vector<uint32_t> *binary);

// Only OpDecorate and OpMemberDecorate are supported.
// Missing or malformed SPIR-V fails even when the expected count is zero.
testing::AssertionResult VerifySpirvDecorationCount(const std::vector<uint32_t> &binary,
                                                    spv::Op instruction,
                                                    spv::Decoration decoration,
                                                    size_t expectedCount);
}  // namespace angle

#endif  // TESTS_TEST_UTILS_SPIRV_TEST_UTILS_H_
