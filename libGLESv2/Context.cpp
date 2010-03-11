//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Context.cpp: Implements the gl::Context class, managing all GL state and performing
// rendering operations. It is the GLES2 specific implementation of EGLContext.

#include "Context.h"

#include <algorithm>

#include "main.h"
#include "Display.h"
#include "Buffer.h"
#include "Shader.h"
#include "Program.h"
#include "Texture.h"
#include "FrameBuffer.h"
#include "RenderBuffer.h"
#include "mathutil.h"
#include "utilities.h"
#include "geometry/backend.h"
#include "geometry/VertexDataManager.h"
#include "geometry/dx9.h"

#undef near
#undef far

namespace gl
{
Context::Context(const egl::Config *config)
    : mConfig(config)
{
    setClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    depthClearValue = 1.0f;
    stencilClearValue = 0;

    cullFace = false;
    cullMode = GL_BACK;
    frontFace = GL_CCW;
    depthTest = false;
    depthFunc = GL_LESS;
    blend = false;
    sourceBlendRGB = GL_ONE;
    sourceBlendAlpha = GL_ONE;
    destBlendRGB = GL_ZERO;
    destBlendAlpha = GL_ZERO;
    blendEquationRGB = GL_FUNC_ADD;
    blendEquationAlpha = GL_FUNC_ADD;
    blendColor.red = 0;
    blendColor.green = 0;
    blendColor.blue = 0;
    blendColor.alpha = 0;
    stencilTest = false;
    stencilFunc = GL_ALWAYS;
    stencilRef = 0;
    stencilMask = -1;
    stencilWritemask = -1;
    stencilBackFunc = GL_ALWAYS;
    stencilBackRef = 0;
    stencilBackMask = - 1;
    stencilBackWritemask = -1;
    stencilFail = GL_KEEP;
    stencilPassDepthFail = GL_KEEP;
    stencilPassDepthPass = GL_KEEP;
    stencilBackFail = GL_KEEP;
    stencilBackPassDepthFail = GL_KEEP;
    stencilBackPassDepthPass = GL_KEEP;
    polygonOffsetFill = false;
    sampleAlphaToCoverage = false;
    sampleCoverage = false;
    sampleCoverageValue = 1.0f;
    sampleCoverageInvert = GL_FALSE;
    scissorTest = false;
    dither = true;

    viewportX = 0;
    viewportY = 0;
    viewportWidth = config->mDisplayMode.Width;
    viewportHeight = config->mDisplayMode.Height;
    zNear = 0.0f;
    zFar = 1.0f;

    scissorX = 0;
    scissorY = 0;
    scissorWidth = config->mDisplayMode.Width;
    scissorHeight = config->mDisplayMode.Height;

    colorMaskRed = true;
    colorMaskGreen = true;
    colorMaskBlue = true;
    colorMaskAlpha = true;
    depthMask = true;

    // [OpenGL ES 2.0.24] section 3.7 page 83:
    // In the initial state, TEXTURE_2D and TEXTURE_CUBE_MAP have twodimensional
    // and cube map texture state vectors respectively associated with them.
    // In order that access to these initial textures not be lost, they are treated as texture
    // objects all of whose names are 0.

    mTexture2DZero = new Texture2D();
    mTextureCubeMapZero = new TextureCubeMap();

    mColorbufferZero = NULL;
    mDepthbufferZero = NULL;
    mStencilbufferZero = NULL;

    activeSampler = 0;
    arrayBuffer = 0;
    elementArrayBuffer = 0;
    bindTextureCubeMap(0);
    bindTexture2D(0);
    bindFramebuffer(0);
    bindRenderbuffer(0);

    for (int sampler = 0; sampler < MAX_TEXTURE_IMAGE_UNITS; sampler++)
    {
        samplerTexture[sampler] = 0;
    }

    currentProgram = 0;

    mBufferBackEnd = NULL;
    mVertexDataManager = NULL;

    mInvalidEnum = false;
    mInvalidValue = false;
    mInvalidOperation = false;
    mOutOfMemory = false;
    mInvalidFramebufferOperation = false;
}

Context::~Context()
{
    currentProgram = 0;

    delete mTexture2DZero;
    delete mTextureCubeMapZero;

    delete mColorbufferZero;
    delete mDepthbufferZero;
    delete mStencilbufferZero;

    delete mBufferBackEnd;
    delete mVertexDataManager;

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

    while (!mFramebufferMap.empty())
    {
        deleteFramebuffer(mFramebufferMap.begin()->first);
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

void Context::makeCurrent(egl::Display *display, egl::Surface *surface)
{
    IDirect3DDevice9 *device = display->getDevice();

    if (!mBufferBackEnd)
    {
        mBufferBackEnd = new Dx9BackEnd(device);
        mVertexDataManager = new VertexDataManager(this, mBufferBackEnd);
    }

    // Wrap the existing Direct3D 9 resources into GL objects and assign them to the '0' names
    IDirect3DSurface9 *defaultRenderTarget = surface->getRenderTarget();
    IDirect3DSurface9 *defaultDepthStencil = NULL;
    device->GetDepthStencilSurface(&defaultDepthStencil);

    Framebuffer *framebufferZero = new Framebuffer();
    Colorbuffer *colorbufferZero = new Colorbuffer(defaultRenderTarget);
    Depthbuffer *depthbufferZero = new Depthbuffer(defaultDepthStencil);
    Stencilbuffer *stencilbufferZero = new Stencilbuffer(defaultDepthStencil);

    setFramebufferZero(framebufferZero);
    setColorbufferZero(colorbufferZero);
    setDepthbufferZero(depthbufferZero);
    setStencilbufferZero(stencilbufferZero);

    framebufferZero->setColorbuffer(GL_RENDERBUFFER, 0);
    framebufferZero->setDepthbuffer(GL_RENDERBUFFER, 0);
    framebufferZero->setStencilbuffer(GL_RENDERBUFFER, 0);

    defaultRenderTarget->Release();

    if (defaultDepthStencil)
    {
        defaultDepthStencil->Release();
    }
}

void Context::setClearColor(float red, float green, float blue, float alpha)
{
    colorClearValue.red = red;
    colorClearValue.green = green;
    colorClearValue.blue = blue;
    colorClearValue.alpha = alpha;
}

void Context::setClearDepth(float depth)
{
    depthClearValue = depth;
}

void Context::setClearStencil(int stencil)
{
    stencilClearValue = stencil;
}

// Returns an unused buffer name
GLuint Context::createBuffer()
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
GLuint Context::createShader(GLenum type)
{
    unsigned int handle = 1;

    while (mShaderMap.find(handle) != mShaderMap.end() || mProgramMap.find(handle) != mProgramMap.end())   // Shared name space
    {
        handle++;
    }

    if (type == GL_VERTEX_SHADER)
    {
        mShaderMap[handle] = new VertexShader();
    }
    else if (type == GL_FRAGMENT_SHADER)
    {
        mShaderMap[handle] = new FragmentShader();
    }
    else UNREACHABLE();

    return handle;
}

// Returns an unused program/shader name
GLuint Context::createProgram()
{
    unsigned int handle = 1;

    while (mProgramMap.find(handle) != mProgramMap.end() || mShaderMap.find(handle) != mShaderMap.end())   // Shared name space
    {
        handle++;
    }

    mProgramMap[handle] = new Program();

    return handle;
}

// Returns an unused texture name
GLuint Context::createTexture()
{
    unsigned int handle = 1;

    while (mTextureMap.find(handle) != mTextureMap.end())
    {
        handle++;
    }

    mTextureMap[handle] = NULL;

    return handle;
}

// Returns an unused framebuffer name
GLuint Context::createFramebuffer()
{
    unsigned int handle = 1;

    while (mFramebufferMap.find(handle) != mFramebufferMap.end())
    {
        handle++;
    }

    mFramebufferMap[handle] = NULL;

    return handle;
}

// Returns an unused renderbuffer name
GLuint Context::createRenderbuffer()
{
    unsigned int handle = 1;

    while (mRenderbufferMap.find(handle) != mRenderbufferMap.end())
    {
        handle++;
    }

    mRenderbufferMap[handle] = NULL;

    return handle;
}

void Context::deleteBuffer(GLuint buffer)
{
    BufferMap::iterator bufferObject = mBufferMap.find(buffer);

    if (bufferObject != mBufferMap.end())
    {
        detachBuffer(buffer);

        delete bufferObject->second;
        mBufferMap.erase(bufferObject);
    }
}

void Context::deleteShader(GLuint shader)
{
    ShaderMap::iterator shaderObject = mShaderMap.find(shader);

    if (shaderObject != mShaderMap.end())
    {
        if (!shaderObject->second->isAttached())
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

void Context::deleteProgram(GLuint program)
{
    ProgramMap::iterator programObject = mProgramMap.find(program);

    if (programObject != mProgramMap.end())
    {
        if (program != currentProgram)
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

void Context::deleteTexture(GLuint texture)
{
    TextureMap::iterator textureObject = mTextureMap.find(texture);

    if (textureObject != mTextureMap.end())
    {
        detachTexture(texture);

        if (texture != 0)
        {
            delete textureObject->second;
        }

        mTextureMap.erase(textureObject);
    }
}

void Context::deleteFramebuffer(GLuint framebuffer)
{
    FramebufferMap::iterator framebufferObject = mFramebufferMap.find(framebuffer);

    if (framebufferObject != mFramebufferMap.end())
    {
        detachFramebuffer(framebuffer);

        delete framebufferObject->second;
        mFramebufferMap.erase(framebufferObject);
    }
}

void Context::deleteRenderbuffer(GLuint renderbuffer)
{
    RenderbufferMap::iterator renderbufferObject = mRenderbufferMap.find(renderbuffer);

    if (renderbufferObject != mRenderbufferMap.end())
    {
        detachRenderbuffer(renderbuffer);

        delete renderbufferObject->second;
        mRenderbufferMap.erase(renderbufferObject);
    }
}

void Context::bindArrayBuffer(unsigned int buffer)
{
    if (buffer != 0 && !getBuffer(buffer))
    {
        mBufferMap[buffer] = new Buffer(mBufferBackEnd);
    }

    arrayBuffer = buffer;
}

void Context::bindElementArrayBuffer(unsigned int buffer)
{
    if (buffer != 0 && !getBuffer(buffer))
    {
        mBufferMap[buffer] = new Buffer(mBufferBackEnd);
    }

    elementArrayBuffer = buffer;
}

void Context::bindTexture2D(GLuint texture)
{
    if (!getTexture(texture) || texture == 0)
    {
        if (texture != 0)
        {
            mTextureMap[texture] = new Texture2D();
        }
        else   // Special case: 0 refers to different initial textures based on the target
        {
            mTextureMap[0] = mTexture2DZero;
        }
    }

    texture2D = texture;

    samplerTexture[activeSampler] = texture;
}

void Context::bindTextureCubeMap(GLuint texture)
{
    if (!getTexture(texture) || texture == 0)
    {
        if (texture != 0)
        {
            mTextureMap[texture] = new TextureCubeMap();
        }
        else   // Special case: 0 refers to different initial textures based on the target
        {
            mTextureMap[0] = mTextureCubeMapZero;
        }
    }

    textureCubeMap = texture;

    samplerTexture[activeSampler] = texture;
}

void Context::bindFramebuffer(GLuint framebuffer)
{
    if (!getFramebuffer(framebuffer))
    {
        mFramebufferMap[framebuffer] = new Framebuffer();
    }

    this->framebuffer = framebuffer;
}

void Context::bindRenderbuffer(GLuint renderbuffer)
{
    if (renderbuffer != 0 && !getRenderbuffer(renderbuffer))
    {
        mRenderbufferMap[renderbuffer] = new Renderbuffer();
    }

    this->renderbuffer = renderbuffer;
}

void Context::useProgram(GLuint program)
{
    Program *programObject = getCurrentProgram();

    if (programObject && programObject->isFlaggedForDeletion())
    {
        deleteProgram(currentProgram);
    }

    currentProgram = program;
}

void Context::setFramebufferZero(Framebuffer *buffer)
{
    delete mFramebufferMap[0];
    mFramebufferMap[0] = buffer;
}

void Context::setColorbufferZero(Colorbuffer *buffer)
{
    delete mColorbufferZero;
    mColorbufferZero = buffer;
}

void Context::setDepthbufferZero(Depthbuffer *buffer)
{
    delete mDepthbufferZero;
    mDepthbufferZero = buffer;
}

void Context::setStencilbufferZero(Stencilbuffer *buffer)
{
    delete mStencilbufferZero;
    mStencilbufferZero = buffer;
}

void Context::setRenderbuffer(Renderbuffer *buffer)
{
    delete mRenderbufferMap[renderbuffer];
    mRenderbufferMap[renderbuffer] = buffer;
}

Buffer *Context::getBuffer(unsigned int handle)
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

Shader *Context::getShader(unsigned int handle)
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

Program *Context::getProgram(unsigned int handle)
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

Texture *Context::getTexture(unsigned int handle)
{
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

Framebuffer *Context::getFramebuffer(unsigned int handle)
{
    FramebufferMap::iterator framebuffer = mFramebufferMap.find(handle);

    if (framebuffer == mFramebufferMap.end())
    {
        return NULL;
    }
    else
    {
        return framebuffer->second;
    }
}

Renderbuffer *Context::getRenderbuffer(unsigned int handle)
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

Colorbuffer *Context::getColorbuffer(GLuint handle)
{
    if (handle != 0)
    {
        Renderbuffer *renderbuffer = getRenderbuffer(handle);

        if (renderbuffer && renderbuffer->isColorbuffer())
        {
            return static_cast<Colorbuffer*>(renderbuffer);
        }
    }
    else   // Special case: 0 refers to different initial render targets based on the attachment type
    {
        return mColorbufferZero;
    }

    return NULL;
}

Depthbuffer *Context::getDepthbuffer(GLuint handle)
{
    if (handle != 0)
    {
        Renderbuffer *renderbuffer = getRenderbuffer(handle);

        if (renderbuffer && renderbuffer->isDepthbuffer())
        {
            return static_cast<Depthbuffer*>(renderbuffer);
        }
    }
    else   // Special case: 0 refers to different initial render targets based on the attachment type
    {
        return mDepthbufferZero;
    }

    return NULL;
}

Stencilbuffer *Context::getStencilbuffer(GLuint handle)
{
    if (handle != 0)
    {
        Renderbuffer *renderbuffer = getRenderbuffer(handle);

        if (renderbuffer && renderbuffer->isStencilbuffer())
        {
            return static_cast<Stencilbuffer*>(renderbuffer);
        }
    }
    else
    {
        return mStencilbufferZero;
    }

    return NULL;
}

Buffer *Context::getArrayBuffer()
{
    return getBuffer(arrayBuffer);
}

Buffer *Context::getElementArrayBuffer()
{
    return getBuffer(elementArrayBuffer);
}

Program *Context::getCurrentProgram()
{
    return getProgram(currentProgram);
}

Texture2D *Context::getTexture2D()
{
    if (texture2D == 0)   // Special case: 0 refers to different initial textures based on the target
    {
        return mTexture2DZero;
    }

    return (Texture2D*)getTexture(texture2D);
}

TextureCubeMap *Context::getTextureCubeMap()
{
    if (textureCubeMap == 0)   // Special case: 0 refers to different initial textures based on the target
    {
        return mTextureCubeMapZero;
    }

    return (TextureCubeMap*)getTexture(textureCubeMap);
}

Texture *Context::getSamplerTexture(unsigned int sampler)
{
    return getTexture(samplerTexture[sampler]);
}

Framebuffer *Context::getFramebuffer()
{
    return getFramebuffer(framebuffer);
}

// Applies the render target surface, depth stencil surface, viewport rectangle and
// scissor rectangle to the Direct3D 9 device
bool Context::applyRenderTarget(bool ignoreViewport)
{
    IDirect3DDevice9 *device = getDevice();
    Framebuffer *framebufferObject = getFramebuffer();

    if (!framebufferObject || framebufferObject->completeness() != GL_FRAMEBUFFER_COMPLETE)
    {
        return false;
    }

    IDirect3DSurface9 *renderTarget = framebufferObject->getRenderTarget();
    IDirect3DSurface9 *depthStencil = framebufferObject->getDepthStencil();

    device->SetRenderTarget(0, renderTarget);
    device->SetDepthStencilSurface(depthStencil);

    D3DVIEWPORT9 viewport;
    D3DSURFACE_DESC desc;
    renderTarget->GetDesc(&desc);

    if (ignoreViewport)
    {
        viewport.X = 0;
        viewport.Y = 0;
        viewport.Width = desc.Width;
        viewport.Height = desc.Height;
        viewport.MinZ = 0.0f;
        viewport.MaxZ = 1.0f;
    }
    else
    {
        viewport.X = std::max(viewportX, 0);
        viewport.Y = std::max(viewportY, 0);
        viewport.Width = std::min(viewportWidth, (int)desc.Width - (int)viewport.X);
        viewport.Height = std::min(viewportHeight, (int)desc.Height - (int)viewport.Y);
        viewport.MinZ = clamp01(zNear);
        viewport.MaxZ = clamp01(zFar);
    }

    device->SetViewport(&viewport);

    if (scissorTest)
    {
        RECT rect = {scissorX,
                     scissorY,
                     scissorX + scissorWidth,
                     scissorY + scissorHeight};

        device->SetScissorRect(&rect);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
    }
    else
    {
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    }

    if (currentProgram)
    {
        D3DSURFACE_DESC description;
        renderTarget->GetDesc(&description);
        Program *programObject = getCurrentProgram();

        GLuint halfPixelSize = programObject->getUniformLocation("gl_HalfPixelSize");
        GLfloat xy[2] = {1.0f / description.Width, 1.0f / description.Height};
        programObject->setUniform2fv(halfPixelSize, 1, (GLfloat*)&xy);

        GLuint near = programObject->getUniformLocation("gl_DepthRange.near");
        programObject->setUniform1fv(near, 1, &zNear);

        GLuint far = programObject->getUniformLocation("gl_DepthRange.far");
        programObject->setUniform1fv(far, 1, &zFar);

        GLuint diff = programObject->getUniformLocation("gl_DepthRange.diff");
        GLfloat zDiff = zFar - zNear;
        programObject->setUniform1fv(diff, 1, &zDiff);
    }

    return true;
}

// Applies the fixed-function state (culling, depth test, alpha blending, stenciling, etc) to the Direct3D 9 device
void Context::applyState()
{
    IDirect3DDevice9 *device = getDevice();

    if (cullFace)
    {
        device->SetRenderState(D3DRS_CULLMODE, es2dx::ConvertCullMode(cullMode, frontFace));
    }
    else
    {
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    }

    if (depthTest)
    {
        device->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
        device->SetRenderState(D3DRS_ZFUNC, es2dx::ConvertComparison(depthFunc));
    }
    else
    {
        device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    }

    if (blend)
    {
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

        if (sourceBlendRGB != GL_CONSTANT_ALPHA && sourceBlendRGB != GL_ONE_MINUS_CONSTANT_ALPHA &&
            destBlendRGB != GL_CONSTANT_ALPHA && destBlendRGB != GL_ONE_MINUS_CONSTANT_ALPHA)
        {
            device->SetRenderState(D3DRS_BLENDFACTOR, es2dx::ConvertColor(blendColor));
        }
        else
        {
            device->SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_RGBA(unorm<8>(blendColor.alpha),
                                                                    unorm<8>(blendColor.alpha),
                                                                    unorm<8>(blendColor.alpha),
                                                                    unorm<8>(blendColor.alpha)));
        }

        device->SetRenderState(D3DRS_SRCBLEND, es2dx::ConvertBlendFunc(sourceBlendRGB));
        device->SetRenderState(D3DRS_DESTBLEND, es2dx::ConvertBlendFunc(destBlendRGB));
        device->SetRenderState(D3DRS_BLENDOP, es2dx::ConvertBlendOp(blendEquationRGB));

        if (sourceBlendRGB != sourceBlendAlpha || destBlendRGB != destBlendAlpha || blendEquationRGB != blendEquationAlpha)
        {
            device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);

            device->SetRenderState(D3DRS_SRCBLENDALPHA, es2dx::ConvertBlendFunc(sourceBlendAlpha));
            device->SetRenderState(D3DRS_DESTBLENDALPHA, es2dx::ConvertBlendFunc(destBlendAlpha));
            device->SetRenderState(D3DRS_BLENDOPALPHA, es2dx::ConvertBlendOp(blendEquationAlpha));

        }
        else
        {
            device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
        }
    }
    else
    {
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    }

    if (stencilTest)
    {
        device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
        device->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, TRUE);

        // FIXME: Unsupported by D3D9
        const D3DRENDERSTATETYPE D3DRS_CCW_STENCILREF = D3DRS_STENCILREF;
        const D3DRENDERSTATETYPE D3DRS_CCW_STENCILMASK = D3DRS_STENCILMASK;
        const D3DRENDERSTATETYPE D3DRS_CCW_STENCILWRITEMASK = D3DRS_STENCILWRITEMASK;
        ASSERT(stencilRef == stencilBackRef);
        ASSERT(stencilMask == stencilBackMask);
        ASSERT(stencilWritemask == stencilBackWritemask);

        device->SetRenderState(frontFace == GL_CCW ? D3DRS_STENCILWRITEMASK : D3DRS_CCW_STENCILWRITEMASK, stencilWritemask);
        device->SetRenderState(frontFace == GL_CCW ? D3DRS_STENCILFUNC : D3DRS_CCW_STENCILFUNC, es2dx::ConvertComparison(stencilFunc));

        device->SetRenderState(frontFace == GL_CCW ? D3DRS_STENCILREF : D3DRS_CCW_STENCILREF, stencilRef);   // FIXME: Clamp to range
        device->SetRenderState(frontFace == GL_CCW ? D3DRS_STENCILMASK : D3DRS_CCW_STENCILMASK, stencilMask);

        device->SetRenderState(frontFace == GL_CCW ? D3DRS_STENCILFAIL : D3DRS_CCW_STENCILFAIL, es2dx::ConvertStencilOp(stencilFail));
        device->SetRenderState(frontFace == GL_CCW ? D3DRS_STENCILZFAIL : D3DRS_CCW_STENCILZFAIL, es2dx::ConvertStencilOp(stencilPassDepthFail));
        device->SetRenderState(frontFace == GL_CCW ? D3DRS_STENCILPASS : D3DRS_CCW_STENCILPASS, es2dx::ConvertStencilOp(stencilPassDepthPass));

        device->SetRenderState(frontFace == GL_CW ? D3DRS_STENCILWRITEMASK : D3DRS_CCW_STENCILWRITEMASK, stencilBackWritemask);
        device->SetRenderState(frontFace == GL_CW ? D3DRS_STENCILFUNC : D3DRS_CCW_STENCILFUNC, es2dx::ConvertComparison(stencilBackFunc));

        device->SetRenderState(frontFace == GL_CW ? D3DRS_STENCILREF : D3DRS_CCW_STENCILREF, stencilBackRef);   // FIXME: Clamp to range
        device->SetRenderState(frontFace == GL_CW ? D3DRS_STENCILMASK : D3DRS_CCW_STENCILMASK, stencilBackMask);

        device->SetRenderState(frontFace == GL_CW ? D3DRS_STENCILFAIL : D3DRS_CCW_STENCILFAIL, es2dx::ConvertStencilOp(stencilBackFail));
        device->SetRenderState(frontFace == GL_CW ? D3DRS_STENCILZFAIL : D3DRS_CCW_STENCILZFAIL, es2dx::ConvertStencilOp(stencilBackPassDepthFail));
        device->SetRenderState(frontFace == GL_CW ? D3DRS_STENCILPASS : D3DRS_CCW_STENCILPASS, es2dx::ConvertStencilOp(stencilBackPassDepthPass));
    }
    else
    {
        device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    }

    device->SetRenderState(D3DRS_COLORWRITEENABLE, es2dx::ConvertColorMask(colorMaskRed, colorMaskGreen, colorMaskBlue, colorMaskAlpha));
    device->SetRenderState(D3DRS_ZWRITEENABLE, depthMask ? TRUE : FALSE);

    if (polygonOffsetFill)
    {
        UNIMPLEMENTED();   // FIXME
    }

    if (sampleAlphaToCoverage)
    {
        UNIMPLEMENTED();   // FIXME
    }

    if (sampleCoverage)
    {
        UNIMPLEMENTED();   // FIXME: Ignore when SAMPLE_BUFFERS is not one
    }

    device->SetRenderState(D3DRS_DITHERENABLE, dither ? TRUE : FALSE);
}

// Fill in the programAttribute field of the array of TranslatedAttributes based on the active GLSL program.
void Context::lookupAttributeMapping(TranslatedAttribute *attributes)
{
    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        if (attributes[i].enabled)
        {
            attributes[i].programAttribute = getCurrentProgram()->getInputMapping(i);
        }
    }
}

// The indices parameter to glDrawElements can have two interpretations:
// - as a pointer into client memory
// - as an offset into the current GL_ELEMENT_ARRAY_BUFFER buffer
// Handle these cases here and return a pointer to the index data.
const Index *Context::adjustIndexPointer(const void *indices)
{
    if (elementArrayBuffer)
    {
        Buffer *buffer = getBuffer(elementArrayBuffer);
        return reinterpret_cast<const Index*>(static_cast<unsigned char*>(buffer->data()) + reinterpret_cast<GLsizei>(indices));
    }
    else
    {
        return static_cast<const Index*>(indices);
    }
}

void Context::applyVertexBuffer(GLint first, GLsizei count)
{
    TranslatedAttribute translated[MAX_VERTEX_ATTRIBS];

    mVertexDataManager->preRenderValidate(first, count, translated);

    lookupAttributeMapping(translated);

    mBufferBackEnd->preDraw(translated);
}

void Context::applyVertexBuffer(GLsizei count, const void *indices, GLenum indexType)
{
    TranslatedAttribute translated[MAX_VERTEX_ATTRIBS];

    mVertexDataManager->preRenderValidate(adjustIndexPointer(indices), count, translated);

    lookupAttributeMapping(translated);

    mBufferBackEnd->preDraw(translated);
}

// Applies the indices and element array bindings to the Direct3D 9 device
void Context::applyIndexBuffer(const void *indices, GLsizei count)
{
    GLsizei length = count * sizeof(Index);

    IDirect3DDevice9 *device = getDevice();

    IDirect3DIndexBuffer9 *indexBuffer = NULL;
    void *data;

    HRESULT result = device->CreateIndexBuffer(length, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &indexBuffer, NULL);

    if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
    {
        return error(GL_OUT_OF_MEMORY);
    }

    ASSERT(SUCCEEDED(result));

    if (indexBuffer)
    {
        indexBuffer->Lock(0, length, &data, 0);
        memcpy(data, adjustIndexPointer(indices), length);
        indexBuffer->Unlock();

        device->SetIndices(indexBuffer);
        indexBuffer->Release();   // Will only effectively be deleted when no longer in use
    }

    startIndex = 0;
}

// Applies the shaders and shader constants to the Direct3D 9 device
void Context::applyShaders()
{
    IDirect3DDevice9 *device = getDevice();
    Program *programObject = getCurrentProgram();
    IDirect3DVertexShader9 *vertexShader = programObject->getVertexShader();
    IDirect3DPixelShader9 *pixelShader = programObject->getPixelShader();

    device->SetVertexShader(vertexShader);
    device->SetPixelShader(pixelShader);

    programObject->applyUniforms();
}

// Applies the textures and sampler states to the Direct3D 9 device
void Context::applyTextures()
{
    IDirect3DDevice9 *device = getDevice();
    Program *programObject = getCurrentProgram();

    for (int sampler = 0; sampler < MAX_TEXTURE_IMAGE_UNITS; sampler++)
    {
        unsigned int textureUnit = programObject->getSamplerMapping(sampler);
        Texture *texture = getSamplerTexture(textureUnit);

        if (texture && texture->isComplete())
        {
            GLenum wrapS = texture->getWrapS();
            GLenum wrapT = texture->getWrapT();
            GLenum minFilter = texture->getMinFilter();
            GLenum magFilter = texture->getMagFilter();

            device->SetSamplerState(sampler, D3DSAMP_ADDRESSU, es2dx::ConvertTextureWrap(wrapS));
            device->SetSamplerState(sampler, D3DSAMP_ADDRESSV, es2dx::ConvertTextureWrap(wrapT));

            device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, es2dx::ConvertMagFilter(magFilter));
            D3DTEXTUREFILTERTYPE d3dMinFilter, d3dMipFilter;
            es2dx::ConvertMinFilter(minFilter, &d3dMinFilter, &d3dMipFilter);
            device->SetSamplerState(sampler, D3DSAMP_MINFILTER, d3dMinFilter);
            device->SetSamplerState(sampler, D3DSAMP_MIPFILTER, d3dMipFilter);

            device->SetTexture(sampler, texture->getTexture());
        }
    }
}

void Context::readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels)
{
    Framebuffer *framebuffer = getFramebuffer();
    IDirect3DSurface9 *renderTarget = framebuffer->getRenderTarget();
    IDirect3DDevice9 *device = getDevice();

    D3DSURFACE_DESC desc;
    renderTarget->GetDesc(&desc);

    IDirect3DSurface9 *systemSurface;
    HRESULT result = device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &systemSurface, NULL);

    if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
    {
        return error(GL_OUT_OF_MEMORY);
    }

    ASSERT(SUCCEEDED(result));

    if (desc.MultiSampleType != D3DMULTISAMPLE_NONE)
    {
        UNIMPLEMENTED();   // FIXME: Requires resolve using StretchRect into non-multisampled render target
    }

    result = device->GetRenderTargetData(renderTarget, systemSurface);

    if (result == D3DERR_DRIVERINTERNALERROR)
    {
        systemSurface->Release();

        return error(GL_OUT_OF_MEMORY);
    }

    if (FAILED(result))
    {
        UNREACHABLE();
        systemSurface->Release();

        return;   // No sensible error to generate
    }

    D3DLOCKED_RECT lock;
    RECT rect = {std::max(x, 0),
                 std::max(y, 0),
                 std::min(x + width, (int)desc.Width),
                 std::min(y + height, (int)desc.Height)};

    result = systemSurface->LockRect(&lock, &rect, D3DLOCK_READONLY);

    if (FAILED(result))
    {
        UNREACHABLE();
        systemSurface->Release();

        return;   // No sensible error to generate
    }

    unsigned char *source = (unsigned char*)lock.pBits;
    unsigned char *dest = (unsigned char*)pixels;

    for (int j = 0; j < rect.bottom - rect.top; j++)
    {
        for (int i = 0; i < rect.right - rect.left; i++)
        {
            float r;
            float g;
            float b;
            float a;

            switch (desc.Format)
            {
              case D3DFMT_R5G6B5:
                {
                    unsigned short rgb = *(unsigned short*)(source + 2 * i + j * lock.Pitch);

                    a = 1.0f;
                    b = (rgb & 0x001F) * (1.0f / 0x001F);
                    g = (rgb & 0x07E0) * (1.0f / 0x07E0);
                    r = (rgb & 0xF800) * (1.0f / 0xF800);
                }
                break;
              case D3DFMT_X1R5G5B5:
                {
                    unsigned short xrgb = *(unsigned short*)(source + 2 * i + j * lock.Pitch);

                    a = 1.0f;
                    b = (xrgb & 0x001F) * (1.0f / 0x001F);
                    g = (xrgb & 0x03E0) * (1.0f / 0x03E0);
                    r = (xrgb & 0x7C00) * (1.0f / 0x7C00);
                }
                break;
              case D3DFMT_A1R5G5B5:
                {
                    unsigned short argb = *(unsigned short*)(source + 2 * i + j * lock.Pitch);

                    a = (argb & 0x8000) ? 1.0f : 0.0f;
                    b = (argb & 0x001F) * (1.0f / 0x001F);
                    g = (argb & 0x03E0) * (1.0f / 0x03E0);
                    r = (argb & 0x7C00) * (1.0f / 0x7C00);
                }
                break;
              case D3DFMT_A8R8G8B8:
                {
                    unsigned int argb = *(unsigned int*)(source + 4 * i + j * lock.Pitch);

                    a = (argb & 0xFF000000) * (1.0f / 0xFF000000);
                    b = (argb & 0x000000FF) * (1.0f / 0x000000FF);
                    g = (argb & 0x0000FF00) * (1.0f / 0x0000FF00);
                    r = (argb & 0x00FF0000) * (1.0f / 0x00FF0000);
                }
                break;
              case D3DFMT_X8R8G8B8:
                {
                    unsigned int xrgb = *(unsigned int*)(source + 4 * i + j * lock.Pitch);

                    a = 1.0f;
                    b = (xrgb & 0x000000FF) * (1.0f / 0x000000FF);
                    g = (xrgb & 0x0000FF00) * (1.0f / 0x0000FF00);
                    r = (xrgb & 0x00FF0000) * (1.0f / 0x00FF0000);
                }
                break;
              case D3DFMT_A2R10G10B10:
                {
                    unsigned int argb = *(unsigned int*)(source + 4 * i + j * lock.Pitch);

                    a = (argb & 0xC0000000) * (1.0f / 0xC0000000);
                    b = (argb & 0x000003FF) * (1.0f / 0x000003FF);
                    g = (argb & 0x000FFC00) * (1.0f / 0x000FFC00);
                    r = (argb & 0x3FF00000) * (1.0f / 0x3FF00000);
                }
                break;
              default:
                UNIMPLEMENTED();   // FIXME
                UNREACHABLE();
            }

            switch (format)
            {
              case GL_RGBA:
                switch (type)
                {
                  case GL_UNSIGNED_BYTE:
                    dest[4 * (i + j * width) + 0] = (unsigned char)(255 * r + 0.5f);
                    dest[4 * (i + j * width) + 1] = (unsigned char)(255 * g + 0.5f);
                    dest[4 * (i + j * width) + 2] = (unsigned char)(255 * b + 0.5f);
                    dest[4 * (i + j * width) + 3] = (unsigned char)(255 * a + 0.5f);
                    break;
                  default: UNREACHABLE();
                }
                break;
              case IMPLEMENTATION_COLOR_READ_FORMAT:
                switch (type)
                {
                  case IMPLEMENTATION_COLOR_READ_TYPE:
                    break;
                  default: UNREACHABLE();
                }
                break;
              default: UNREACHABLE();
            }
        }
    }

    systemSurface->UnlockRect();

    systemSurface->Release();
}

void Context::clear(GLbitfield mask)
{
    IDirect3DDevice9 *device = getDevice();
    DWORD flags = 0;

    if (mask & GL_COLOR_BUFFER_BIT)
    {
        mask &= ~GL_COLOR_BUFFER_BIT;
        flags |= D3DCLEAR_TARGET;
    }

    if (mask & GL_DEPTH_BUFFER_BIT)
    {
        mask &= ~GL_DEPTH_BUFFER_BIT;
        if (depthMask)
        {
            flags |= D3DCLEAR_ZBUFFER;
        }
    }

    Framebuffer *framebufferObject = getFramebuffer();
    IDirect3DSurface9 *depthStencil = framebufferObject->getDepthStencil();

    GLuint stencilUnmasked = 0x0;

    if ((mask & GL_STENCIL_BUFFER_BIT) && depthStencil)
    {
        D3DSURFACE_DESC desc;
        depthStencil->GetDesc(&desc);

        mask &= ~GL_STENCIL_BUFFER_BIT;
        unsigned int stencilSize = es2dx::GetStencilSize(desc.Format);
        stencilUnmasked = (0x1 << stencilSize) - 1;

        if (stencilUnmasked != 0x0)
        {
            flags |= D3DCLEAR_STENCIL;
        }
    }

    if (mask != 0)
    {
        return error(GL_INVALID_VALUE);
    }

    applyRenderTarget(true);   // Clips the clear to the scissor rectangle but not the viewport

    D3DCOLOR color = D3DCOLOR_ARGB(unorm<8>(colorClearValue.alpha), unorm<8>(colorClearValue.red), unorm<8>(colorClearValue.green), unorm<8>(colorClearValue.blue));
    float depth = clamp01(depthClearValue);
    int stencil = stencilClearValue & 0x000000FF;

    IDirect3DSurface9 *renderTarget = framebufferObject->getRenderTarget();

    D3DSURFACE_DESC desc;
    renderTarget->GetDesc(&desc);

    bool alphaUnmasked = (es2dx::GetAlphaSize(desc.Format) == 0) || colorMaskAlpha;

    const bool needMaskedStencilClear = (flags & D3DCLEAR_STENCIL) &&
                                        (stencilWritemask & stencilUnmasked) != stencilUnmasked;
    const bool needMaskedColorClear = (flags & D3DCLEAR_TARGET) &&
                                      !(colorMaskRed && colorMaskGreen &&
                                        colorMaskBlue && alphaUnmasked);

    if (needMaskedColorClear || needMaskedStencilClear)
    {
        device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        device->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
        device->SetRenderState(D3DRS_ZENABLE, FALSE);
        device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
        device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);

        if (flags & D3DCLEAR_TARGET)
        {
            device->SetRenderState(D3DRS_COLORWRITEENABLE, (colorMaskRed   ? D3DCOLORWRITEENABLE_RED   : 0) |
                                                           (colorMaskGreen ? D3DCOLORWRITEENABLE_GREEN : 0) |
                                                           (colorMaskBlue  ? D3DCOLORWRITEENABLE_BLUE  : 0) |
                                                           (colorMaskAlpha ? D3DCOLORWRITEENABLE_ALPHA : 0));
        }
        else
        {
            device->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
        }

        if (stencilUnmasked != 0x0 && (flags & D3DCLEAR_STENCIL))
        {
            device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
            device->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, FALSE);
            device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
            device->SetRenderState(D3DRS_STENCILREF, stencil);
            device->SetRenderState(D3DRS_STENCILWRITEMASK, stencilWritemask);
            device->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_REPLACE);
            device->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_REPLACE);
            device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
        }
        else
        {
            device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        }

        device->SetPixelShader(NULL);
        device->SetVertexShader(NULL);
        device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

        struct Vertex
        {
            float x, y, z, w;
            D3DCOLOR diffuse;
        };

        Vertex quad[4];
        quad[0].x = 0.0f;
        quad[0].y = (float)desc.Height;
        quad[0].z = 0.0f;
        quad[0].w = 1.0f;
        quad[0].diffuse = color;

        quad[1].x = (float)desc.Width;
        quad[1].y = (float)desc.Height;
        quad[1].z = 0.0f;
        quad[1].w = 1.0f;
        quad[1].diffuse = color;

        quad[2].x = 0.0f;
        quad[2].y = 0.0f;
        quad[2].z = 0.0f;
        quad[2].w = 1.0f;
        quad[2].diffuse = color;

        quad[3].x = (float)desc.Width;
        quad[3].y = 0.0f;
        quad[3].z = 0.0f;
        quad[3].w = 1.0f;
        quad[3].diffuse = color;

        device->BeginScene();
        device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, sizeof(Vertex));
        device->EndScene();

        if (flags & D3DCLEAR_ZBUFFER)
        {
            device->SetRenderState(D3DRS_ZENABLE, TRUE);
            device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
            device->Clear(0, NULL, D3DCLEAR_ZBUFFER, color, depth, stencil);
        }
    }
    else
    {
        device->Clear(0, NULL, flags, color, depth, stencil);
    }
}

void Context::drawArrays(GLenum mode, GLint first, GLsizei count)
{
    if (!currentProgram)
    {
        return error(GL_INVALID_OPERATION);
    }

    IDirect3DDevice9 *device = getDevice();
    D3DPRIMITIVETYPE primitiveType;
    int primitiveCount;

    if(!es2dx::ConvertPrimitiveType(mode, count, &primitiveType, &primitiveCount))
        return error(GL_INVALID_ENUM);

    if (primitiveCount <= 0)
    {
        return;
    }

    if (!applyRenderTarget(false))
    {
        return error(GL_INVALID_FRAMEBUFFER_OPERATION);
    }

    applyState();
    applyVertexBuffer(first, count);
    applyShaders();
    applyTextures();

    device->BeginScene();
    device->DrawPrimitive(primitiveType, first, primitiveCount);
    device->EndScene();
}

void Context::drawElements(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
    if (!currentProgram)
    {
        return error(GL_INVALID_OPERATION);
    }

    if (!indices && !elementArrayBuffer)
    {
        return error(GL_INVALID_OPERATION);
    }

    IDirect3DDevice9 *device = getDevice();
    D3DPRIMITIVETYPE primitiveType;
    int primitiveCount;

    if(!es2dx::ConvertPrimitiveType(mode, count, &primitiveType, &primitiveCount))
        return error(GL_INVALID_ENUM);

    if (primitiveCount <= 0)
    {
        return;
    }

    if (!applyRenderTarget(false))
    {
        return error(GL_INVALID_FRAMEBUFFER_OPERATION);
    }

    applyState();
    applyVertexBuffer(count, indices, type);
    applyIndexBuffer(indices, count);
    applyShaders();
    applyTextures();

    device->BeginScene();
    device->DrawIndexedPrimitive(primitiveType, 0, 0, count, startIndex, primitiveCount);
    device->EndScene();
}

void Context::finish()
{
    IDirect3DDevice9 *device = getDevice();
    IDirect3DQuery9 *occlusionQuery = NULL;

    HRESULT result = device->CreateQuery(D3DQUERYTYPE_OCCLUSION, &occlusionQuery);

    if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
    {
        return error(GL_OUT_OF_MEMORY);
    }

    ASSERT(SUCCEEDED(result));

    if (occlusionQuery)
    {
        occlusionQuery->Issue(D3DISSUE_BEGIN);

        // Render something outside the render target
        device->SetPixelShader(NULL);
        device->SetVertexShader(NULL);
        device->SetFVF(D3DFVF_XYZRHW);
        float data[4] = {-1.0f, -1.0f, -1.0f, 1.0f};
        device->BeginScene();
        device->DrawPrimitiveUP(D3DPT_POINTLIST, 1, data, sizeof(data));
        device->EndScene();

        occlusionQuery->Issue(D3DISSUE_END);

        while (occlusionQuery->GetData(NULL, 0, D3DGETDATA_FLUSH) == S_FALSE)
        {
            // Keep polling, but allow other threads to do something useful first
            Sleep(0);
        }

        occlusionQuery->Release();
    }
}

void Context::flush()
{
    IDirect3DDevice9 *device = getDevice();
    IDirect3DQuery9 *eventQuery = NULL;

    HRESULT result = device->CreateQuery(D3DQUERYTYPE_EVENT, &eventQuery);

    if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
    {
        return error(GL_OUT_OF_MEMORY);
    }

    ASSERT(SUCCEEDED(result));

    if (eventQuery)
    {
        eventQuery->Issue(D3DISSUE_END);

        while (eventQuery->GetData(NULL, 0, D3DGETDATA_FLUSH) == S_FALSE)
        {
            // Keep polling, but allow other threads to do something useful first
            Sleep(0);
        }

        eventQuery->Release();
    }
}

void Context::recordInvalidEnum()
{
    mInvalidEnum = true;
}

void Context::recordInvalidValue()
{
    mInvalidValue = true;
}

void Context::recordInvalidOperation()
{
    mInvalidOperation = true;
}

void Context::recordOutOfMemory()
{
    mOutOfMemory = true;
}

void Context::recordInvalidFramebufferOperation()
{
    mInvalidFramebufferOperation = true;
}

// Get one of the recorded errors and clear its flag, if any.
// [OpenGL ES 2.0.24] section 2.5 page 13.
GLenum Context::getError()
{
    if (mInvalidEnum)
    {
        mInvalidEnum = false;

        return GL_INVALID_ENUM;
    }

    if (mInvalidValue)
    {
        mInvalidValue = false;

        return GL_INVALID_VALUE;
    }

    if (mInvalidOperation)
    {
        mInvalidOperation = false;

        return GL_INVALID_OPERATION;
    }

    if (mOutOfMemory)
    {
        mOutOfMemory = false;

        return GL_OUT_OF_MEMORY;
    }

    if (mInvalidFramebufferOperation)
    {
        mInvalidFramebufferOperation = false;

        return GL_INVALID_FRAMEBUFFER_OPERATION;
    }

    return GL_NO_ERROR;
}

void Context::detachBuffer(GLuint buffer)
{
    // [OpenGL ES 2.0.24] section 2.9 page 22:
    // If a buffer object is deleted while it is bound, all bindings to that object in the current context
    // (i.e. in the thread that called Delete-Buffers) are reset to zero.

    if (arrayBuffer == buffer)
    {
        arrayBuffer = 0;
    }

    if (elementArrayBuffer == buffer)
    {
        elementArrayBuffer = 0;
    }

    for (int attribute = 0; attribute < MAX_VERTEX_ATTRIBS; attribute++)
    {
        if (vertexAttribute[attribute].mBoundBuffer == buffer)
        {
            vertexAttribute[attribute].mBoundBuffer = 0;
        }
    }
}

void Context::detachTexture(GLuint texture)
{
    // [OpenGL ES 2.0.24] section 3.8 page 84:
    // If a texture object is deleted, it is as if all texture units which are bound to that texture object are
    // rebound to texture object zero

    for (int sampler = 0; sampler < MAX_TEXTURE_IMAGE_UNITS; sampler++)
    {
        if (samplerTexture[sampler] == texture)
        {
            samplerTexture[sampler] = 0;
        }
    }

    // [OpenGL ES 2.0.24] section 4.4 page 112:
    // If a texture object is deleted while its image is attached to the currently bound framebuffer, then it is
    // as if FramebufferTexture2D had been called, with a texture of 0, for each attachment point to which this
    // image was attached in the currently bound framebuffer.

    Framebuffer *framebuffer = getFramebuffer();

    if (framebuffer)
    {
        framebuffer->detachTexture(texture);
    }
}

void Context::detachFramebuffer(GLuint framebuffer)
{
    // [OpenGL ES 2.0.24] section 4.4 page 107:
    // If a framebuffer that is currently bound to the target FRAMEBUFFER is deleted, it is as though
    // BindFramebuffer had been executed with the target of FRAMEBUFFER and framebuffer of zero.

    if (this->framebuffer == framebuffer)
    {
        bindFramebuffer(0);
    }
}

void Context::detachRenderbuffer(GLuint renderbuffer)
{
    // [OpenGL ES 2.0.24] section 4.4 page 109:
    // If a renderbuffer that is currently bound to RENDERBUFFER is deleted, it is as though BindRenderbuffer
    // had been executed with the target RENDERBUFFER and name of zero.

    if (this->renderbuffer == renderbuffer)
    {
        bindRenderbuffer(0);
    }

    // [OpenGL ES 2.0.24] section 4.4 page 111:
    // If a renderbuffer object is deleted while its image is attached to the currently bound framebuffer,
    // then it is as if FramebufferRenderbuffer had been called, with a renderbuffer of 0, for each attachment
    // point to which this image was attached in the currently bound framebuffer.

    Framebuffer *framebuffer = getFramebuffer();

    if (framebuffer)
    {
        framebuffer->detachRenderbuffer(renderbuffer);
    }
}
}

extern "C"
{
gl::Context *glCreateContext(const egl::Config *config)
{
    return new gl::Context(config);
}

void glDestroyContext(gl::Context *context)
{
    delete context;

    if (context == gl::getContext())
    {
        gl::makeCurrent(NULL, NULL, NULL);
    }
}

void glMakeCurrent(gl::Context *context, egl::Display *display, egl::Surface *surface)
{
    gl::makeCurrent(context, display, surface);
}

gl::Context *glGetCurrentContext()
{
    return gl::getContext();
}
}
