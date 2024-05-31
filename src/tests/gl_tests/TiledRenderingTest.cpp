//
// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

namespace angle
{
class TiledRenderingTest : public ANGLETest<>
{
  protected:
    TiledRenderingTest()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }
};

// Validate that the extension entry points generate errors when the extension is not available.
TEST_P(TiledRenderingTest, ExtensionDisabled)
{
    ANGLE_SKIP_TEST_IF(IsGLExtensionEnabled("GL_QCOM_tiled_rendering"));
    glStartTilingQCOM(0, 0, 1, 1, GL_COLOR_BUFFER_BIT0_QCOM);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    glEndTilingQCOM(GL_COLOR_BUFFER_BIT0_QCOM);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
}

ANGLE_INSTANTIATE_TEST_ES2_AND_ES3(TiledRenderingTest);

}  // namespace angle
