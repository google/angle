//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// GetTexLevelParameterTest.cpp : Tests of the GL_ANGLE_get_tex_level_parameter extension.

#include "test_utils/ANGLETest.h"

#include "test_utils/gl_raii.h"

namespace angle
{

class GetTexLevelParameterTest : public ANGLETest<>
{
  protected:
    GetTexLevelParameterTest()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setExtensionsEnabled(false);
    }
};

// Extension is requestable so it should be disabled by default.
TEST_P(GetTexLevelParameterTest, ExtensionStringExposed)
{
    EXPECT_FALSE(IsGLExtensionEnabled("GL_ANGLE_get_tex_level_parameter"));

    if (IsGLExtensionRequestable("GL_ANGLE_get_tex_level_parameter"))
    {
        glRequestExtensionANGLE("GL_ANGLE_get_tex_level_parameter");
        EXPECT_GL_NO_ERROR();

        EXPECT_TRUE(IsGLExtensionEnabled("GL_ANGLE_get_tex_level_parameter"));
    }
}

// Test that extension entry points are rejected with extension disabled
TEST_P(GetTexLevelParameterTest, NoExtension)
{
    GLint resulti = 0;
    glGetTexLevelParameterivANGLE(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &resulti);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    GLfloat resultf = 0;
    glGetTexLevelParameterfvANGLE(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &resultf);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
}

// Test valid targets for level queries
TEST_P(GetTexLevelParameterTest, Targets)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_ANGLE_get_tex_level_parameter"));

    GLint result = 0;

    // These tests use default texture objects.

    {
        glGetTexLevelParameterivANGLE(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &result);
        EXPECT_GL_NO_ERROR();
    }

    {
        glGetTexLevelParameterivANGLE(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_WIDTH, &result);
        EXPECT_GL_NO_ERROR();
        glGetTexLevelParameterivANGLE(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_TEXTURE_WIDTH, &result);
        EXPECT_GL_NO_ERROR();
        glGetTexLevelParameterivANGLE(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_TEXTURE_WIDTH, &result);
        EXPECT_GL_NO_ERROR();
        glGetTexLevelParameterivANGLE(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_TEXTURE_WIDTH, &result);
        EXPECT_GL_NO_ERROR();
        glGetTexLevelParameterivANGLE(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_TEXTURE_WIDTH, &result);
        EXPECT_GL_NO_ERROR();
        glGetTexLevelParameterivANGLE(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_TEXTURE_WIDTH, &result);
        EXPECT_GL_NO_ERROR();
    }

    {
        glGetTexLevelParameterivANGLE(GL_TEXTURE_2D_ARRAY, 0, GL_TEXTURE_WIDTH, &result);
        if (getClientMajorVersion() < 3)
        {
            EXPECT_GL_ERROR(GL_INVALID_ENUM);
        }
        else
        {
            EXPECT_GL_NO_ERROR();
        }
    }

    {
        glGetTexLevelParameterivANGLE(GL_TEXTURE_2D_MULTISAMPLE, 0, GL_TEXTURE_WIDTH, &result);
        if (getClientMajorVersion() < 3 || getClientMinorVersion() < 1)
        {
            EXPECT_GL_ERROR(GL_INVALID_ENUM);
            if (EnsureGLExtensionEnabled("GL_ANGLE_texture_multisample"))
            {
                glGetTexLevelParameterivANGLE(GL_TEXTURE_2D_MULTISAMPLE, 0, GL_TEXTURE_WIDTH,
                                              &result);
                EXPECT_GL_NO_ERROR();
            }
        }
        else
        {
            EXPECT_GL_NO_ERROR();
        }
    }

    {
        glGetTexLevelParameterivANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0, GL_TEXTURE_WIDTH,
                                      &result);
        if (getClientMajorVersion() < 3 || getClientMinorVersion() < 2)
        {
            EXPECT_GL_ERROR(GL_INVALID_ENUM);
            if (EnsureGLExtensionEnabled("GL_OES_texture_storage_multisample_2d_array"))
            {
                glGetTexLevelParameterivANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0, GL_TEXTURE_WIDTH,
                                              &result);
                EXPECT_GL_NO_ERROR();
            }
        }
        else
        {
            EXPECT_GL_NO_ERROR();
        }
    }

    {
        glGetTexLevelParameterivANGLE(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &result);
        if (getClientMajorVersion() < 3)
        {
            EXPECT_GL_ERROR(GL_INVALID_ENUM);
            if (EnsureGLExtensionEnabled("GL_OES_texture_3d"))
            {
                glGetTexLevelParameterivANGLE(GL_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &result);
                EXPECT_GL_NO_ERROR();
            }
        }
        else
        {
            EXPECT_GL_NO_ERROR();
        }
    }

    {
        glGetTexLevelParameterivANGLE(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_TEXTURE_WIDTH, &result);
        if (getClientMajorVersion() < 3 || getClientMinorVersion() < 2)
        {
            EXPECT_GL_ERROR(GL_INVALID_ENUM);
            if (EnsureGLExtensionEnabled("GL_EXT_texture_cube_map_array") ||
                EnsureGLExtensionEnabled("GL_OES_texture_cube_map_array"))
            {
                glGetTexLevelParameterivANGLE(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_TEXTURE_WIDTH,
                                              &result);
                EXPECT_GL_NO_ERROR();
            }
        }
        else
        {
            EXPECT_GL_NO_ERROR();
        }
    }

    {
        glGetTexLevelParameterivANGLE(GL_TEXTURE_BUFFER, 0, GL_TEXTURE_WIDTH, &result);
        if (getClientMajorVersion() < 3 || getClientMinorVersion() < 2)
        {
            EXPECT_GL_ERROR(GL_INVALID_ENUM);
            if (EnsureGLExtensionEnabled("GL_EXT_texture_buffer") ||
                EnsureGLExtensionEnabled("GL_OES_texture_buffer"))
            {
                glGetTexLevelParameterivANGLE(GL_TEXTURE_BUFFER, 0, GL_TEXTURE_WIDTH, &result);
                EXPECT_GL_NO_ERROR();
            }
        }
        else
        {
            EXPECT_GL_NO_ERROR();
        }
    }
}

// Test various queries exposed by GL_ANGLE_get_tex_level_parameter
TEST_P(GetTexLevelParameterTest, Queries)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_ANGLE_get_tex_level_parameter"));

    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    {
        GLint width = 0;
        glGetTexLevelParameterivANGLE(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
        EXPECT_GL_NO_ERROR();
        EXPECT_EQ(1, width);
    }

    {
        GLint height = 0;
        glGetTexLevelParameterivANGLE(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
        EXPECT_GL_NO_ERROR();
        EXPECT_EQ(2, height);
    }

    {
        GLint internalFormat = 0;
        glGetTexLevelParameterivANGLE(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT,
                                      &internalFormat);
        EXPECT_GL_NO_ERROR();
        EXPECT_GLENUM_EQ(GL_RGBA, internalFormat);
    }

    {
        GLint depth = -1;
        glGetTexLevelParameterivANGLE(GL_TEXTURE_2D, 0, GL_TEXTURE_DEPTH, &depth);
        if (getClientMajorVersion() < 3)
        {
            EXPECT_GL_ERROR(GL_INVALID_ENUM);
            if (EnsureGLExtensionEnabled("GL_OES_texture_3d"))
            {
                glGetTexLevelParameterivANGLE(GL_TEXTURE_2D, 0, GL_TEXTURE_DEPTH, &depth);
                EXPECT_GL_NO_ERROR();
                EXPECT_EQ(depth, 1);
            }
        }
        else
        {
            EXPECT_GL_NO_ERROR();
            EXPECT_EQ(depth, 1);
        }
    }

    if (getClientMajorVersion() >= 3)
    {
        GLint samples = -1;
        glGetTexLevelParameterivANGLE(GL_TEXTURE_2D, 0, GL_TEXTURE_SAMPLES, &samples);
        if (getClientMinorVersion() < 1)
        {
            EXPECT_GL_ERROR(GL_INVALID_ENUM);
        }
        else
        {
            EXPECT_GL_NO_ERROR();
            EXPECT_EQ(samples, 0);
        }

        GLint fixedLocations = 0;
        glGetTexLevelParameterivANGLE(GL_TEXTURE_2D, 0, GL_TEXTURE_FIXED_SAMPLE_LOCATIONS,
                                      &fixedLocations);
        if (getClientMinorVersion() < 1)
        {
            EXPECT_GL_ERROR(GL_INVALID_ENUM);
        }
        else
        {
            EXPECT_GL_NO_ERROR();
            EXPECT_TRUE(fixedLocations);
        }

        if (getClientMinorVersion() < 1 && EnsureGLExtensionEnabled("GL_ANGLE_texture_multisample"))
        {
            glGetTexLevelParameterivANGLE(GL_TEXTURE_2D, 0, GL_TEXTURE_SAMPLES, &samples);
            EXPECT_GL_NO_ERROR();
            EXPECT_EQ(samples, 0);

            glGetTexLevelParameterivANGLE(GL_TEXTURE_2D, 0, GL_TEXTURE_FIXED_SAMPLE_LOCATIONS,
                                          &fixedLocations);
            EXPECT_GL_NO_ERROR();
            EXPECT_TRUE(fixedLocations);
        }
    }
}

// Use this to select which configurations (e.g. which renderer, which GLES major version) these
// tests should be run against.
ANGLE_INSTANTIATE_TEST_ES2_AND_ES3_AND_ES31_AND_ES32(GetTexLevelParameterTest);
}  // namespace angle
