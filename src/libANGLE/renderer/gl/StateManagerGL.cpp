//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// StateManagerGL.h: Defines a class for caching applied OpenGL state

#include "libANGLE/renderer/gl/StateManagerGL.h"

#include "libANGLE/Data.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/VertexArray.h"
#include "libANGLE/renderer/gl/FramebufferGL.h"
#include "libANGLE/renderer/gl/FunctionsGL.h"
#include "libANGLE/renderer/gl/ProgramGL.h"
#include "libANGLE/renderer/gl/TextureGL.h"
#include "libANGLE/renderer/gl/VertexArrayGL.h"

namespace rx
{

StateManagerGL::StateManagerGL(const FunctionsGL *functions, const gl::Caps &rendererCaps)
    : mFunctions(functions),
      mProgram(0),
      mVAO(0),
      mBuffers(),
      mTextureUnitIndex(0),
      mTextures(),
      mUnpackAlignment(4),
      mUnpackRowLength(0),
      mFramebuffers(),
      mRenderbuffer(0),
      mScissor(0, 0, 0, 0),
      mViewport(0, 0, 0, 0),
      mClearColor(0.0f, 0.0f, 0.0f, 0.0f),
      mColorMaskRed(true),
      mColorMaskGreen(true),
      mColorMaskBlue(true),
      mColorMaskAlpha(true),
      mClearDepth(1.0f),
      mDepthMask(true),
      mClearStencil(0),
      mStencilMask(static_cast<GLuint>(-1))
{
    ASSERT(mFunctions);

    mTextures[GL_TEXTURE_2D].resize(rendererCaps.maxCombinedTextureImageUnits);
    mTextures[GL_TEXTURE_CUBE_MAP].resize(rendererCaps.maxCombinedTextureImageUnits);
    mTextures[GL_TEXTURE_2D_ARRAY].resize(rendererCaps.maxCombinedTextureImageUnits);
    mTextures[GL_TEXTURE_3D].resize(rendererCaps.maxCombinedTextureImageUnits);

    mFramebuffers[GL_READ_FRAMEBUFFER] = 0;
    mFramebuffers[GL_DRAW_FRAMEBUFFER] = 0;
}

void StateManagerGL::useProgram(GLuint program)
{
    if (mProgram != program)
    {
        mProgram = program;
        mFunctions->useProgram(mProgram);
    }
}

void StateManagerGL::bindVertexArray(GLuint vao)
{
    if (mVAO != vao)
    {
        mVAO = vao;
        mFunctions->bindVertexArray(vao);
    }
}

void StateManagerGL::bindBuffer(GLenum type, GLuint buffer)
{
    if (mBuffers[type] != buffer)
    {
        mBuffers[type] = buffer;
        mFunctions->bindBuffer(type, buffer);
    }
}

void StateManagerGL::activeTexture(size_t unit)
{
    if (mTextureUnitIndex != unit)
    {
        mTextureUnitIndex = unit;
        mFunctions->activeTexture(GL_TEXTURE0 + mTextureUnitIndex);
    }
}

void StateManagerGL::bindTexture(GLenum type, GLuint texture)
{
    if (mTextures[type][mTextureUnitIndex] != texture)
    {
        mTextures[type][mTextureUnitIndex] = texture;
        mFunctions->bindTexture(type, texture);
    }
}

void StateManagerGL::setPixelUnpackState(GLint alignment, GLint rowLength)
{
    if (mUnpackAlignment != alignment)
    {
        mUnpackAlignment = alignment;
        mFunctions->pixelStorei(GL_UNPACK_ALIGNMENT, mUnpackAlignment);
    }

    if (mUnpackRowLength != rowLength)
    {
        mUnpackRowLength = rowLength;
        mFunctions->pixelStorei(GL_UNPACK_ROW_LENGTH, mUnpackRowLength);
    }
}

void StateManagerGL::bindFramebuffer(GLenum type, GLuint framebuffer)
{
    if (mFramebuffers[type] != framebuffer)
    {
        mFramebuffers[type] = framebuffer;
        mFunctions->bindFramebuffer(type, framebuffer);
    }
}

void StateManagerGL::bindRenderbuffer(GLenum type, GLuint renderbuffer)
{
    ASSERT(type == GL_RENDERBUFFER);
    if (mRenderbuffer != renderbuffer)
    {
        mRenderbuffer = renderbuffer;
        mFunctions->bindRenderbuffer(type, mRenderbuffer);
    }
}

void StateManagerGL::setClearState(const gl::State &state, GLbitfield mask)
{
    // Only apply the state required to do a clear
    setScissor(state.getScissor());
    setViewport(state.getViewport());

    if ((mask & GL_COLOR_BUFFER_BIT) != 0)
    {
        setClearColor(state.getColorClearValue());

        const gl::BlendState &blendState = state.getBlendState();
        setColorMask(blendState.colorMaskRed, blendState.colorMaskGreen, blendState.colorMaskBlue, blendState.colorMaskAlpha);
    }

    if ((mask & GL_DEPTH_BUFFER_BIT) != 0)
    {
        setClearDepth(state.getDepthClearValue());
        setDepthMask(state.getDepthStencilState().depthMask);
    }

    if ((mask & GL_STENCIL_BUFFER_BIT) != 0)
    {
        setClearStencil(state.getStencilClearValue());
        setStencilMask(state.getDepthStencilState().stencilMask);
    }
}

gl::Error StateManagerGL::setDrawArraysState(const gl::Data &data, GLint first, GLsizei count)
{
    const gl::State &state = *data.state;

    const gl::VertexArray *vao = state.getVertexArray();
    const VertexArrayGL *vaoGL = GetImplAs<VertexArrayGL>(vao);
    vaoGL->syncDrawArraysState(first, count);
    bindVertexArray(vaoGL->getVertexArrayID());

    return setGenericDrawState(data);
}

gl::Error StateManagerGL::setDrawElementsState(const gl::Data &data, GLsizei count, GLenum type, const GLvoid *indices,
                                               const GLvoid **outIndices)
{
    const gl::State &state = *data.state;

    const gl::VertexArray *vao = state.getVertexArray();
    const VertexArrayGL *vaoGL = GetImplAs<VertexArrayGL>(vao);

    gl::Error error = vaoGL->syncDrawElementsState(count, type, indices, outIndices);
    if (error.isError())
    {
        return error;
    }

    bindVertexArray(vaoGL->getVertexArrayID());

    return setGenericDrawState(data);
}

gl::Error StateManagerGL::setGenericDrawState(const gl::Data &data)
{
    const gl::State &state = *data.state;
    const gl::Caps &caps = *data.caps;

    const gl::Program *program = state.getProgram();
    const ProgramGL *programGL = GetImplAs<ProgramGL>(program);
    useProgram(programGL->getProgramID());

    // TODO: Only apply textures referenced by the program.
    for (auto textureTypeIter = mTextures.begin(); textureTypeIter != mTextures.end(); textureTypeIter++)
    {
        GLenum textureType = textureTypeIter->first;

        // Determine if this texture type can exist in the source context
        bool validTextureType = (textureType == GL_TEXTURE_2D || textureType == GL_TEXTURE_CUBE_MAP ||
                                 (textureType == GL_TEXTURE_2D_ARRAY && data.clientVersion >= 3) ||
                                 (textureType == GL_TEXTURE_3D && data.clientVersion >= 3));

        const std::vector<GLuint> &textureVector = textureTypeIter->second;
        for (size_t textureUnitIndex = 0; textureUnitIndex < textureVector.size(); textureUnitIndex++)
        {
            const gl::Texture *texture = nullptr;

            bool validTextureUnit = textureUnitIndex < caps.maxCombinedTextureImageUnits;
            if (validTextureType && validTextureUnit)
            {
                texture = state.getSamplerTexture(textureUnitIndex, textureType);
            }

            if (texture != nullptr)
            {
                const TextureGL *textureGL = GetImplAs<TextureGL>(texture);
                if (mTextures[textureType][textureUnitIndex] != textureGL->getTextureID())
                {
                    activeTexture(textureUnitIndex);
                    textureGL->syncSamplerState(texture->getSamplerState());

                    bindTexture(textureType, textureGL->getTextureID());
                }

                // TODO: apply sampler object if one is bound
            }
            else
            {
                if (mTextures[textureType][textureUnitIndex] != 0)
                {
                    activeTexture(textureUnitIndex);
                    bindTexture(textureType, 0);
                }
            }
        }
    }

    const gl::Framebuffer *framebuffer = state.getDrawFramebuffer();
    const FramebufferGL *framebufferGL = GetImplAs<FramebufferGL>(framebuffer);
    bindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferGL->getFramebufferID());

    setScissor(state.getScissor());
    setViewport(state.getViewport());

    const gl::BlendState &blendState = state.getBlendState();
    setColorMask(blendState.colorMaskRed, blendState.colorMaskGreen, blendState.colorMaskBlue, blendState.colorMaskAlpha);

    const gl::DepthStencilState &depthStencilState = state.getDepthStencilState();
    setDepthMask(depthStencilState.depthMask);
    setStencilMask(depthStencilState.stencilMask);

    return gl::Error(GL_NO_ERROR);
}

void StateManagerGL::setScissor(const gl::Rectangle &scissor)
{
    if (scissor != mScissor)
    {
        mScissor = scissor;
        mFunctions->scissor(mScissor.x, mScissor.y, mScissor.width, mScissor.height);
    }
}

void StateManagerGL::setViewport(const gl::Rectangle &viewport)
{
    if (viewport != mViewport)
    {
        mViewport = viewport;
        mFunctions->viewport(mViewport.x, mViewport.y, mViewport.width, mViewport.height);
    }
}

void StateManagerGL::setClearColor(const gl::ColorF &clearColor)
{
    if (mClearColor != clearColor)
    {
        mClearColor = clearColor;
        mFunctions->clearColor(mClearColor.red, mClearColor.green, mClearColor.blue, mClearColor.alpha);
    }
}

void StateManagerGL::setColorMask(bool red, bool green, bool blue, bool alpha)
{
    if (mColorMaskRed != red || mColorMaskGreen != green || mColorMaskBlue != blue || mColorMaskAlpha != alpha)
    {
        mColorMaskRed = red;
        mColorMaskGreen = green;
        mColorMaskBlue = blue;
        mColorMaskAlpha = alpha;
        mFunctions->colorMask(mColorMaskRed, mColorMaskGreen, mColorMaskBlue, mColorMaskAlpha);
    }
}

void StateManagerGL::setClearDepth(float clearDepth)
{
    if (mClearDepth != clearDepth)
    {
        mClearDepth = clearDepth;
        mFunctions->clearDepth(mClearDepth);
    }
}

void StateManagerGL::setDepthMask(bool mask)
{
    if (mDepthMask != mask)
    {
        mDepthMask = mask;
        mFunctions->depthMask(mDepthMask);
    }
}

void StateManagerGL::setClearStencil(GLint clearStencil)
{
    if (mClearStencil != clearStencil)
    {
        mClearStencil = clearStencil;
        mFunctions->clearStencil(mClearStencil);
    }
}

void StateManagerGL::setStencilMask(GLuint mask)
{
    if (mStencilMask != mask)
    {
        mStencilMask = mask;
        mFunctions->stencilMask(mStencilMask);
    }
}

}
