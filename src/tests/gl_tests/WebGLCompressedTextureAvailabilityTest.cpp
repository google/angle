//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// WebGLCompressedTextureAvailabilityTest:
//   Compressed formats must be enabled on platforms that always support them.
//   Compressed formats must be disabled on platforms that never support them.
//

#include "test_utils/ANGLETest.h"

using namespace angle;

namespace
{

class WebGLCompressedTextureAvailabilityTest : public ANGLETest
{
  public:
    WebGLCompressedTextureAvailabilityTest() { setWebGLCompatibilityEnabled(true); }
};

// Test compressed formats availability
TEST_P(WebGLCompressedTextureAvailabilityTest, Test)
{
    const char kDXT1[]     = "GL_EXT_texture_compression_dxt1";
    const char kDXT3[]     = "GL_ANGLE_texture_compression_dxt3";
    const char kDXT5[]     = "GL_ANGLE_texture_compression_dxt5";
    const char kS3TCSRGB[] = "GL_EXT_texture_compression_s3tc_srgb";
    const char kRGTC[]     = "GL_EXT_texture_compression_rgtc";
    const char kBPTC[]     = "GL_EXT_texture_compression_bptc";

    const char kETC1[] = "GL_OES_compressed_ETC1_RGB8_texture";
    const char kETC2[] = "GL_ANGLE_compressed_texture_etc";

    const char kASTCLDR[] = "GL_KHR_texture_compression_astc_ldr";
    const char kASTCHDR[] = "GL_KHR_texture_compression_astc_hdr";

    const char kPVRTC1[] = "GL_IMG_texture_compression_pvrtc";

    if (IsD3D())
    {
        EXPECT_TRUE(EnsureGLExtensionEnabled(kDXT1));
        EXPECT_TRUE(EnsureGLExtensionEnabled(kDXT3));
        EXPECT_TRUE(EnsureGLExtensionEnabled(kDXT5));
        EXPECT_TRUE(EnsureGLExtensionEnabled(kS3TCSRGB));

        if (IsD3D9())
        {
            EXPECT_FALSE(EnsureGLExtensionEnabled(kRGTC));
            EXPECT_FALSE(EnsureGLExtensionEnabled(kBPTC));
        }
        else
        {
            EXPECT_TRUE(EnsureGLExtensionEnabled(kRGTC));
        }

        EXPECT_FALSE(EnsureGLExtensionEnabled(kETC1));
        EXPECT_FALSE(EnsureGLExtensionEnabled(kETC2));
        EXPECT_FALSE(EnsureGLExtensionEnabled(kASTCLDR));
        EXPECT_FALSE(EnsureGLExtensionEnabled(kASTCHDR));
        EXPECT_FALSE(EnsureGLExtensionEnabled(kPVRTC1));
    }
    else if (IsMetal())
    {
        if (IsOSX())
        {
            EXPECT_TRUE(EnsureGLExtensionEnabled(kDXT1));
            EXPECT_TRUE(EnsureGLExtensionEnabled(kDXT3));
            EXPECT_TRUE(EnsureGLExtensionEnabled(kDXT5));
            EXPECT_TRUE(EnsureGLExtensionEnabled(kS3TCSRGB));
            EXPECT_TRUE(EnsureGLExtensionEnabled(kRGTC));
            EXPECT_TRUE(EnsureGLExtensionEnabled(kBPTC));

            if (IsApple())
            {
                // M1 or newer
                EXPECT_TRUE(EnsureGLExtensionEnabled(kETC1));
                EXPECT_TRUE(EnsureGLExtensionEnabled(kETC2));
                EXPECT_TRUE(EnsureGLExtensionEnabled(kASTCLDR));
                EXPECT_TRUE(EnsureGLExtensionEnabled(kASTCHDR));
                EXPECT_TRUE(EnsureGLExtensionEnabled(kPVRTC1));
            }
            else
            {
                // macOS with non-Apple GPU
                EXPECT_FALSE(EnsureGLExtensionEnabled(kETC1));
                EXPECT_FALSE(EnsureGLExtensionEnabled(kETC2));
                EXPECT_FALSE(EnsureGLExtensionEnabled(kASTCLDR));
                EXPECT_FALSE(EnsureGLExtensionEnabled(kASTCHDR));
                EXPECT_FALSE(EnsureGLExtensionEnabled(kPVRTC1));
            }
        }
        else
        {
            // Need proper Catalyst detection to assert formats here.
        }
    }
}

ANGLE_INSTANTIATE_TEST_ES2_AND_ES3(WebGLCompressedTextureAvailabilityTest);

}  // namespace
