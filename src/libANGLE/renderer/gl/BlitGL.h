//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BlitGL.h: Defines the BlitGL class, a helper for blitting textures

#ifndef LIBANGLE_RENDERER_GL_BLITGL_H_
#define LIBANGLE_RENDERER_GL_BLITGL_H_

#include "angle_gl.h"
#include "common/angleutils.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/Error.h"

#include <map>

namespace gl
{
class Framebuffer;
struct ImageIndex;
}

namespace rx
{

class FramebufferGL;
class FunctionsGL;
class RenderbufferGL;
class StateManagerGL;
class TextureGL;
struct WorkaroundsGL;

class BlitGL : angle::NonCopyable
{
  public:
    BlitGL(const FunctionsGL *functions,
           const WorkaroundsGL &workarounds,
           StateManagerGL *stateManager);
    ~BlitGL();

    gl::Error copyImageToLUMAWorkaroundTexture(const gl::Context *context,
                                               GLuint texture,
                                               gl::TextureType textureType,
                                               gl::TextureTarget target,
                                               GLenum lumaFormat,
                                               size_t level,
                                               const gl::Rectangle &sourceArea,
                                               GLenum internalFormat,
                                               gl::Framebuffer *source);

    gl::Error copySubImageToLUMAWorkaroundTexture(const gl::Context *context,
                                                  GLuint texture,
                                                  gl::TextureType textureType,
                                                  gl::TextureTarget target,
                                                  GLenum lumaFormat,
                                                  size_t level,
                                                  const gl::Offset &destOffset,
                                                  const gl::Rectangle &sourceArea,
                                                  gl::Framebuffer *source);

    gl::Error blitColorBufferWithShader(const gl::Framebuffer *source,
                                        const gl::Framebuffer *dest,
                                        const gl::Rectangle &sourceArea,
                                        const gl::Rectangle &destArea,
                                        GLenum filter);

    gl::ErrorOrResult<bool> copySubTexture(const gl::Context *context,
                                           TextureGL *source,
                                           size_t sourceLevel,
                                           GLenum sourceComponentType,
                                           TextureGL *dest,
                                           gl::TextureTarget destTarget,
                                           size_t destLevel,
                                           GLenum destComponentType,
                                           const gl::Extents &sourceSize,
                                           const gl::Rectangle &sourceArea,
                                           const gl::Offset &destOffset,
                                           bool needsLumaWorkaround,
                                           GLenum lumaFormat,
                                           bool unpackFlipY,
                                           bool unpackPremultiplyAlpha,
                                           bool unpackUnmultiplyAlpha);

    gl::Error copySubTextureCPUReadback(const gl::Context *context,
                                        TextureGL *source,
                                        size_t sourceLevel,
                                        GLenum sourceComponentType,
                                        TextureGL *dest,
                                        gl::TextureTarget destTarget,
                                        size_t destLevel,
                                        GLenum destFormat,
                                        GLenum destType,
                                        const gl::Rectangle &sourceArea,
                                        const gl::Offset &destOffset,
                                        bool unpackFlipY,
                                        bool unpackPremultiplyAlpha,
                                        bool unpackUnmultiplyAlpha);

    gl::ErrorOrResult<bool> copyTexSubImage(TextureGL *source,
                                            size_t sourceLevel,
                                            TextureGL *dest,
                                            gl::TextureTarget destTarget,
                                            size_t destLevel,
                                            const gl::Rectangle &sourceArea,
                                            const gl::Offset &destOffset);

    gl::ErrorOrResult<bool> clearRenderableTexture(TextureGL *source,
                                                   GLenum sizedInternalFormat,
                                                   int numTextureLayers,
                                                   const gl::ImageIndex &imageIndex);

    gl::Error clearRenderbuffer(RenderbufferGL *source, GLenum sizedInternalFormat);

    gl::Error clearFramebuffer(FramebufferGL *source);

    gl::Error initializeResources();

  private:
    void orphanScratchTextures();
    void setScratchTextureParameter(GLenum param, GLenum value);

    const FunctionsGL *mFunctions;
    const WorkaroundsGL &mWorkarounds;
    StateManagerGL *mStateManager;

    struct BlitProgram
    {
        GLuint program                = 0;
        GLint sourceTextureLocation   = -1;
        GLint scaleLocation           = -1;
        GLint offsetLocation          = -1;
        GLint multiplyAlphaLocation   = -1;
        GLint unMultiplyAlphaLocation = -1;
    };

    enum class BlitProgramType
    {
        FLOAT_TO_FLOAT,
        FLOAT_TO_UINT,
        UINT_TO_UINT,
    };

    static BlitProgramType getBlitProgramType(GLenum sourceComponentType, GLenum destComponentType);
    gl::Error getBlitProgram(BlitProgramType type, BlitProgram **program);

    std::map<BlitProgramType, BlitProgram> mBlitPrograms;

    GLuint mScratchTextures[2];
    GLuint mScratchFBO;

    GLuint mVAO;
    GLuint mVertexBuffer;
};
}

#endif  // LIBANGLE_RENDERER_GL_BLITGL_H_
