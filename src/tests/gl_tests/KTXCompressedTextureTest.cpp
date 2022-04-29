//
// Copyright 2022 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// KTXCompressedTextureTest.cpp: Tests of reading compressed texture stored in
// .ktx formats

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

#include "media/pixel.inc"

using namespace angle;

class KTXCompressedTextureTest : public ANGLETest
{
  protected:
    KTXCompressedTextureTest()
    {
        setWindowWidth(768);
        setWindowHeight(512);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }
};

// Verify that ANGLE can store and sample the ETC1 compressed texture stored in
// KTX container
TEST_P(KTXCompressedTextureTest, CompressedTexImageETC1)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_OES_compressed_ETC1_RGB8_texture"));
    ANGLE_GL_PROGRAM(textureProgram, essl1_shaders::vs::Texture2D(),
                     essl1_shaders::fs::Texture2D());
    glUseProgram(textureProgram);

    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_ETC1_RGB8_OES, ktx_etc1_width, ktx_etc1_height, 0,
                           ktx_etc1_size, ktx_etc1_data);

    EXPECT_GL_NO_ERROR();

    GLint textureUniformLocation =
        glGetUniformLocation(textureProgram, essl1_shaders::Texture2DUniform());
    glUniform1i(textureUniformLocation, 0);
    drawQuad(textureProgram.get(), essl1_shaders::PositionAttrib(), 0.5f);

    EXPECT_GL_NO_ERROR();
}

ANGLE_INSTANTIATE_TEST_ES2_AND_ES3(KTXCompressedTextureTest);
