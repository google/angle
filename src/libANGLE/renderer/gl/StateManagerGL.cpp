//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// StateManagerGL.h: Defines a class for caching applied OpenGL state

#include "libANGLE/renderer/gl/StateManagerGL.h"

#include "libANGLE/Data.h"
#include "libANGLE/VertexArray.h"
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
      mUnpackRowLength(0)
{
    ASSERT(mFunctions);

    mTextures[GL_TEXTURE_2D].resize(rendererCaps.maxCombinedTextureImageUnits);
    mTextures[GL_TEXTURE_CUBE_MAP].resize(rendererCaps.maxCombinedTextureImageUnits);
    mTextures[GL_TEXTURE_2D_ARRAY].resize(rendererCaps.maxCombinedTextureImageUnits);
    mTextures[GL_TEXTURE_3D].resize(rendererCaps.maxCombinedTextureImageUnits);
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
    if (mBuffers[type] == 0)
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

void StateManagerGL::setDrawState(const gl::Data &data)
{
    const gl::State &state = *data.state;
    const gl::Caps &caps = *data.caps;

    const gl::VertexArray *vao = state.getVertexArray();
    const VertexArrayGL *vaoGL = GetImplAs<VertexArrayGL>(vao);
    bindVertexArray(vaoGL->getVertexArrayID());

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
}

}
