//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ResourceManager.h : Defines the ResourceManager class, which tracks objects
// shared by multiple GL contexts.

#ifndef LIBGLESV2_RESOURCEMANAGER_H_
#define LIBGLESV2_RESOURCEMANAGER_H_

#define GL_APICALL
#include <GLES3/gl3.h>
#include <GLES2/gl2.h>

#ifdef _MSC_VER
#include <hash_map>
#else
#include <unordered_map>
#endif

#include "common/angleutils.h"
#include "libGLESv2/angletypes.h"
#include "libGLESv2/HandleAllocator.h"

namespace rx
{
class Renderer;
}

namespace gl
{
class Buffer;
class Shader;
class Program;
class Texture;
class Renderbuffer;
class Sampler;
class FenceSync;

class ResourceManager
{
  public:
    explicit ResourceManager(rx::Renderer *renderer);
    ~ResourceManager();

    void addRef();
    void release();

    GLuint createBuffer();
    GLuint createShader(GLenum type);
    GLuint createProgram();
    GLuint createTexture();
    GLuint createRenderbuffer();
    GLuint createSampler();
    GLuint createFenceSync();

    void deleteBuffer(GLuint buffer);
    void deleteShader(GLuint shader);
    void deleteProgram(GLuint program);
    void deleteTexture(GLuint texture);
    void deleteRenderbuffer(GLuint renderbuffer);
    void deleteSampler(GLuint sampler);
    void deleteFenceSync(GLuint fenceSync);

    Buffer *getBuffer(GLuint handle);
    Shader *getShader(GLuint handle);
    Program *getProgram(GLuint handle);
    Texture *getTexture(GLuint handle);
    Renderbuffer *getRenderbuffer(GLuint handle);
    Sampler *getSampler(GLuint handle);
    FenceSync *getFenceSync(GLuint handle);
    
    void setRenderbuffer(GLuint handle, Renderbuffer *renderbuffer);

    void checkBufferAllocation(unsigned int buffer);
    void checkTextureAllocation(GLuint texture, TextureType type);
    void checkRenderbufferAllocation(GLuint renderbuffer);
    void checkSamplerAllocation(GLuint sampler);

    bool isSampler(GLuint sampler);

  private:
    DISALLOW_COPY_AND_ASSIGN(ResourceManager);

    std::size_t mRefCount;
    rx::Renderer *mRenderer;

#ifndef HASH_MAP
# ifdef _MSC_VER
#  define HASH_MAP stdext::hash_map
# else
#  define HASH_MAP std::unordered_map
# endif
#endif

    typedef HASH_MAP<GLuint, Buffer*> BufferMap;
    BufferMap mBufferMap;
    HandleAllocator mBufferHandleAllocator;

    typedef HASH_MAP<GLuint, Shader*> ShaderMap;
    ShaderMap mShaderMap;

    typedef HASH_MAP<GLuint, Program*> ProgramMap;
    ProgramMap mProgramMap;
    HandleAllocator mProgramShaderHandleAllocator;

    typedef HASH_MAP<GLuint, Texture*> TextureMap;
    TextureMap mTextureMap;
    HandleAllocator mTextureHandleAllocator;

    typedef HASH_MAP<GLuint, Renderbuffer*> RenderbufferMap;
    RenderbufferMap mRenderbufferMap;
    HandleAllocator mRenderbufferHandleAllocator;

    HASH_MAP<GLuint, Sampler*> mSamplerMap;
    HandleAllocator mSamplerHandleAllocator;

    HASH_MAP<GLuint, FenceSync*> mFenceSyncMap;
    HandleAllocator mFenceSyncHandleAllocator;
};

}

#endif // LIBGLESV2_RESOURCEMANAGER_H_
