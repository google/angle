//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// VulkanEXTTest:
//   Tests specific to the ANGLE Vulkan extension.
//

#include "test_utils/ANGLETest.h"

using namespace angle;

namespace
{

class VulkanEXTTest : public ANGLETest
{
  public:
    VulkanEXTTest() {}

    // Intentionally do not call base class so we can run EGL in the tests.
    void SetUp() override {}
};

// ANGLE requires that the vulkan validation layers are available.
TEST_P(VulkanEXTTest, ValidationLayersAvaialable)
{
    setVulkanLayersEnabled(true);
    ASSERT_TRUE(getEGLWindow()->initializeGL(GetOSWindow()));
}

ANGLE_INSTANTIATE_TEST(VulkanEXTTest, ES2_VULKAN(), ES3_VULKAN());

}  // anonymous namespace
