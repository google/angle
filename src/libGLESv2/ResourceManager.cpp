//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ResourceManager.cpp: Implements the gl::ResourceManager class, which tracks and 
// retrieves objects which may be shared by multiple Contexts.

#include "libGLESv2/ResourceManager.h"

#include "libGLESv2/Buffer.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/RenderBuffer.h"
#include "libGLESv2/Shader.h"
#include "libGLESv2/Texture.h"

namespace gl
{
ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
    while (!mBufferMap.empty())
    {
        deleteBuffer(mBufferMap.begin()->first);
    }

    while (!mProgramMap.empty())
    {
        deleteProgram(mProgramMap.begin()->first);
    }

    while (!mShaderMap.empty())
    {
        deleteShader(mShaderMap.begin()->first);
    }

    while (!mRenderbufferMap.empty())
    {
        deleteRenderbuffer(mRenderbufferMap.begin()->first);
    }

    while (!mTextureMap.empty())
    {
        deleteTexture(mTextureMap.begin()->first);
    }
}

// Returns an unused buffer name
GLuint ResourceManager::createBuffer()
{
    unsigned int handle = 1;

    while (mBufferMap.find(handle) != mBufferMap.end())
    {
        handle++;
    }

    mBufferMap[handle] = NULL;

    return handle;
}

// Returns an unused shader/program name
GLuint ResourceManager::createShader(GLenum type)
{
    unsigned int handle = 1;

    while (mShaderMap.find(handle) != mShaderMap.end() || mProgramMap.find(handle) != mProgramMap.end())   // Shared name space
    {
        handle++;
    }

    if (type == GL_VERTEX_SHADER)
    {
        mShaderMap[handle] = new VertexShader(this, handle);
    }
    else if (type == GL_FRAGMENT_SHADER)
    {
        mShaderMap[handle] = new FragmentShader(this, handle);
    }
    else UNREACHABLE();

    return handle;
}

// Returns an unused program/shader name
GLuint ResourceManager::createProgram()
{
    unsigned int handle = 1;

    while (mProgramMap.find(handle) != mProgramMap.end() || mShaderMap.find(handle) != mShaderMap.end())   // Shared name space
    {
        handle++;
    }

    mProgramMap[handle] = new Program(this, handle);

    return handle;
}

// Returns an unused texture name
GLuint ResourceManager::createTexture()
{
    unsigned int handle = 1;

    while (mTextureMap.find(handle) != mTextureMap.end())
    {
        handle++;
    }

    mTextureMap[handle] = NULL;

    return handle;
}

// Returns an unused renderbuffer name
GLuint ResourceManager::createRenderbuffer()
{
    unsigned int handle = 1;

    while (mRenderbufferMap.find(handle) != mRenderbufferMap.end())
    {
        handle++;
    }

    mRenderbufferMap[handle] = NULL;

    return handle;
}

// FIXME: shared object deletion needs handling
void ResourceManager::deleteBuffer(GLuint buffer)
{
    BufferMap::iterator bufferObject = mBufferMap.find(buffer);

    if (bufferObject != mBufferMap.end())
    {
        delete bufferObject->second;
        mBufferMap.erase(bufferObject);
    }
}

void ResourceManager::deleteShader(GLuint shader)
{
    ShaderMap::iterator shaderObject = mShaderMap.find(shader);

    if (shaderObject != mShaderMap.end())
    {
        if (shaderObject->second->getRefCount() == 0)
        {
            delete shaderObject->second;
            mShaderMap.erase(shaderObject);
        }
        else
        {
            shaderObject->second->flagForDeletion();
        }
    }
}

void ResourceManager::deleteProgram(GLuint program)
{
    ProgramMap::iterator programObject = mProgramMap.find(program);

    if (programObject != mProgramMap.end())
    {
        if (programObject->second->getRefCount() == 0)
        {
            delete programObject->second;
            mProgramMap.erase(programObject);
        }
        else
        { 
            programObject->second->flagForDeletion();
        }
    }
}

// FIXME: shared object deletion needs handling
void ResourceManager::deleteTexture(GLuint texture)
{
    TextureMap::iterator textureObject = mTextureMap.find(texture);

    if (textureObject != mTextureMap.end())
    {
        if (texture != 0)
        {
            delete textureObject->second;
        }

        mTextureMap.erase(textureObject);
    }
}

// FIXME: shared object deletion needs handling
void ResourceManager::deleteRenderbuffer(GLuint renderbuffer)
{
    RenderbufferMap::iterator renderbufferObject = mRenderbufferMap.find(renderbuffer);

    if (renderbufferObject != mRenderbufferMap.end())
    {
        delete renderbufferObject->second;
        mRenderbufferMap.erase(renderbufferObject);
    }
}

Buffer *ResourceManager::getBuffer(unsigned int handle)
{
    BufferMap::iterator buffer = mBufferMap.find(handle);

    if (buffer == mBufferMap.end())
    {
        return NULL;
    }
    else
    {
        return buffer->second;
    }
}

Shader *ResourceManager::getShader(unsigned int handle)
{
    ShaderMap::iterator shader = mShaderMap.find(handle);

    if (shader == mShaderMap.end())
    {
        return NULL;
    }
    else
    {
        return shader->second;
    }
}

Texture *ResourceManager::getTexture(unsigned int handle)
{
    if (handle == 0) return NULL;

    TextureMap::iterator texture = mTextureMap.find(handle);

    if (texture == mTextureMap.end())
    {
        return NULL;
    }
    else
    {
        return texture->second;
    }
}

Program *ResourceManager::getProgram(unsigned int handle)
{
    ProgramMap::iterator program = mProgramMap.find(handle);

    if (program == mProgramMap.end())
    {
        return NULL;
    }
    else
    {
        return program->second;
    }
}

Renderbuffer *ResourceManager::getRenderbuffer(unsigned int handle)
{
    RenderbufferMap::iterator renderbuffer = mRenderbufferMap.find(handle);

    if (renderbuffer == mRenderbufferMap.end())
    {
        return NULL;
    }
    else
    {
        return renderbuffer->second;
    }
}

void ResourceManager::setRenderbuffer(GLuint handle, Renderbuffer *buffer)
{
    mRenderbufferMap[handle] = buffer;
}

void ResourceManager::checkBufferAllocation(unsigned int buffer)
{
    if (buffer != 0 && !getBuffer(buffer))
    {
        mBufferMap[buffer] = new Buffer();
    }
}

void ResourceManager::checkTextureAllocation(GLuint texture, SamplerType type)
{
    if (!getTexture(texture) && texture != 0)
    {
        if (type == SAMPLER_2D)
        {
            mTextureMap[texture] = new Texture2D();
        }
        else if (type == SAMPLER_CUBE)
        {
            mTextureMap[texture] = new TextureCubeMap();
        }
    }
}

void ResourceManager::checkRenderbufferAllocation(GLuint renderbuffer)
{
    if (renderbuffer != 0 && !getRenderbuffer(renderbuffer))
    {
        mRenderbufferMap[renderbuffer] = new Renderbuffer();
    }
}

}
