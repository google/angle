//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// MultiviewTest:
//   Implementation of helpers for multiview testing.
//

#include "test_utils/MultiviewTest.h"
#include "platform/WorkaroundsD3D.h"

namespace angle
{

GLuint CreateSimplePassthroughProgram(int numViews)
{
    const std::string vsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "layout(num_views = " +
        ToString(numViews) +
        ") in;\n"
        "layout(location=0) in vec2 vPosition;\n"
        "void main()\n"
        "{\n"
        "   gl_PointSize = 1.;\n"
        "   gl_Position = vec4(vPosition.xy, 0.0, 1.0);\n"
        "}\n";

    const std::string fsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "precision mediump float;\n"
        "out vec4 col;\n"
        "void main()\n"
        "{\n"
        "   col = vec4(0,1,0,1);\n"
        "}\n";
    return CompileProgram(vsSource, fsSource);
}

void CreateMultiviewBackingTextures(GLenum multiviewLayout,
                                    int viewWidth,
                                    int height,
                                    int numLayers,
                                    std::vector<GLuint> colorTextures,
                                    GLuint depthTexture,
                                    GLuint depthStencilTexture)
{
    // The same zero data is used to initialize both color and depth/stencil textures.
    std::vector<GLubyte> textureData;
    textureData.resize(viewWidth * height * numLayers * 4, 0u);

    // Create color and depth textures.
    switch (multiviewLayout)
    {
        case GL_FRAMEBUFFER_MULTIVIEW_SIDE_BY_SIDE_ANGLE:
        {
            int textureWidth = viewWidth * numLayers;
            for (auto colorTexture : colorTextures)
            {
                glBindTexture(GL_TEXTURE_2D, colorTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, textureWidth, height, 0, GL_RGBA,
                             GL_UNSIGNED_BYTE, textureData.data());
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            }

            if (depthTexture != 0)
            {
                glBindTexture(GL_TEXTURE_2D, depthTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, textureWidth, height, 0,
                             GL_DEPTH_COMPONENT, GL_FLOAT, textureData.data());
            }
            if (depthStencilTexture != 0)
            {
                glBindTexture(GL_TEXTURE_2D, depthStencilTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, textureWidth, height, 0,
                             GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, textureData.data());
            }
            glBindTexture(GL_TEXTURE_2D, 0);
            break;
        }
        case GL_FRAMEBUFFER_MULTIVIEW_LAYERED_ANGLE:
            for (auto colorTexture : colorTextures)
            {
                glBindTexture(GL_TEXTURE_2D_ARRAY, colorTexture);
                glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, viewWidth, height, numLayers, 0,
                             GL_RGBA, GL_UNSIGNED_BYTE, textureData.data());
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            }

            if (depthTexture != 0)
            {
                glBindTexture(GL_TEXTURE_2D_ARRAY, depthTexture);
                glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32F, viewWidth, height,
                             numLayers, 0, GL_DEPTH_COMPONENT, GL_FLOAT, textureData.data());
            }
            if (depthStencilTexture != 0)
            {
                glBindTexture(GL_TEXTURE_2D_ARRAY, depthStencilTexture);
                glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH24_STENCIL8, viewWidth, height,
                             numLayers, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8,
                             textureData.data());
            }
            glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
            break;
        default:
            UNREACHABLE();
    }
    ASSERT_GL_NO_ERROR();
}

void CreateMultiviewBackingTextures(GLenum multiviewLayout,
                                    int viewWidth,
                                    int height,
                                    int numLayers,
                                    GLuint colorTexture,
                                    GLuint depthTexture,
                                    GLuint depthStencilTexture)
{
    ASSERT(colorTexture != 0u);
    std::vector<GLuint> colorTextures(1, colorTexture);
    CreateMultiviewBackingTextures(multiviewLayout, viewWidth, height, numLayers, colorTextures,
                                   depthTexture, depthStencilTexture);
}

void AttachMultiviewTextures(GLenum target,
                             GLenum multiviewLayout,
                             int viewWidth,
                             int numViews,
                             int baseViewIndex,
                             std::vector<GLuint> colorTextures,
                             GLuint depthTexture,
                             GLuint depthStencilTexture)
{
    ASSERT(multiviewLayout == GL_FRAMEBUFFER_MULTIVIEW_LAYERED_ANGLE || (baseViewIndex == 0));
    ASSERT(depthTexture == 0u || depthStencilTexture == 0u);
    switch (multiviewLayout)
    {
        case GL_FRAMEBUFFER_MULTIVIEW_SIDE_BY_SIDE_ANGLE:
        {
            std::vector<GLint> viewportOffsets(numViews * 2);
            for (int i = 0u; i < numViews; ++i)
            {
                viewportOffsets[i * 2]     = i * viewWidth;
                viewportOffsets[i * 2 + 1] = 0;
            }
            for (size_t i = 0; i < colorTextures.size(); ++i)
            {
                glFramebufferTextureMultiviewSideBySideANGLE(target, GL_COLOR_ATTACHMENT0 + i,
                                                             colorTextures[i], 0, numViews,
                                                             viewportOffsets.data());
            }
            if (depthTexture)
            {
                glFramebufferTextureMultiviewSideBySideANGLE(
                    target, GL_DEPTH_ATTACHMENT, depthTexture, 0, numViews, viewportOffsets.data());
            }
            if (depthStencilTexture)
            {
                glFramebufferTextureMultiviewSideBySideANGLE(target, GL_DEPTH_STENCIL_ATTACHMENT,
                                                             depthStencilTexture, 0, numViews,
                                                             viewportOffsets.data());
            }
            break;
        }
        case GL_FRAMEBUFFER_MULTIVIEW_LAYERED_ANGLE:
            for (size_t i = 0; i < colorTextures.size(); ++i)
            {
                glFramebufferTextureMultiviewLayeredANGLE(
                    target, GL_COLOR_ATTACHMENT0 + i, colorTextures[i], 0, baseViewIndex, numViews);
            }
            if (depthTexture)
            {
                glFramebufferTextureMultiviewLayeredANGLE(target, GL_DEPTH_ATTACHMENT, depthTexture,
                                                          0, baseViewIndex, numViews);
            }
            if (depthStencilTexture)
            {
                glFramebufferTextureMultiviewLayeredANGLE(target, GL_DEPTH_STENCIL_ATTACHMENT,
                                                          depthStencilTexture, 0, baseViewIndex,
                                                          numViews);
            }
            break;
        default:
            UNREACHABLE();
    }
}

void AttachMultiviewTextures(GLenum target,
                             GLenum multiviewLayout,
                             int viewWidth,
                             int numViews,
                             int baseViewIndex,
                             GLuint colorTexture,
                             GLuint depthTexture,
                             GLuint depthStencilTexture)
{
    ASSERT(colorTexture != 0u);
    std::vector<GLuint> colorTextures(1, colorTexture);
    AttachMultiviewTextures(target, multiviewLayout, viewWidth, numViews, baseViewIndex,
                            colorTextures, depthTexture, depthStencilTexture);
}

std::ostream &operator<<(std::ostream &os, const MultiviewImplementationParams &params)
{
    const PlatformParameters &base = static_cast<const PlatformParameters &>(params);
    os << base;
    if (params.mForceUseGeometryShaderOnD3D)
    {
        os << "_force_geom_shader";
    }
    else
    {
        os << "_vertex_shader";
    }
    return os;
}

MultiviewImplementationParams VertexShaderOpenGL(GLint majorVersion, GLint minorVersion)
{
    return MultiviewImplementationParams(majorVersion, minorVersion, false, egl_platform::OPENGL());
}

MultiviewImplementationParams VertexShaderD3D11(GLint majorVersion, GLint minorVersion)
{
    return MultiviewImplementationParams(majorVersion, minorVersion, false, egl_platform::D3D11());
}

MultiviewImplementationParams GeomShaderD3D11(GLint majorVersion, GLint minorVersion)
{
    return MultiviewImplementationParams(majorVersion, minorVersion, true, egl_platform::D3D11());
}

void MultiviewTest::overrideWorkaroundsD3D(WorkaroundsD3D *workarounds)
{
    workarounds->selectViewInGeometryShader = GetParam().mForceUseGeometryShaderOnD3D;
}

}  // namespace angle
