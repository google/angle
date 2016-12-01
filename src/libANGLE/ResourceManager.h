//
// Copyright (c) 2002-2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ResourceManager.h : Defines the ResourceManager classes, which handle allocation and lifetime of
// GL objects.

#ifndef LIBANGLE_RESOURCEMANAGER_H_
#define LIBANGLE_RESOURCEMANAGER_H_

#include "angle_gl.h"
#include "common/angleutils.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/Error.h"
#include "libANGLE/HandleAllocator.h"
#include "libANGLE/HandleRangeAllocator.h"

namespace rx
{
class GLImplFactory;
}

namespace gl
{
class Buffer;
class FenceSync;
struct Limitations;
class Path;
class Program;
class Renderbuffer;
class Sampler;
class Shader;
class Texture;

template <typename HandleAllocatorType>
class ResourceManagerBase : angle::NonCopyable
{
  public:
    ResourceManagerBase();

    void addRef();
    void release();

  protected:
    virtual ~ResourceManagerBase() {}

    HandleAllocatorType mHandleAllocator;

  private:
    size_t mRefCount;
};

template <typename ResourceType, typename HandleAllocatorType>
class TypedResourceManager : public ResourceManagerBase<HandleAllocatorType>
{
  public:
    TypedResourceManager() {}

  protected:
    ~TypedResourceManager() override;

    void deleteObject(GLuint handle);

    ResourceMap<ResourceType> mObjectMap;
};

class BufferManager : public TypedResourceManager<Buffer, HandleAllocator>
{
  public:
    GLuint createBuffer();
    void deleteBuffer(GLuint buffer);
    Buffer *getBuffer(GLuint handle) const;
    Buffer *checkBufferAllocation(rx::GLImplFactory *factory, GLuint handle);
    bool isBufferGenerated(GLuint buffer) const;

  protected:
    ~BufferManager() override {}
};

class ShaderProgramManager : public ResourceManagerBase<HandleAllocator>
{
  public:
    GLuint createShader(rx::GLImplFactory *factory,
                        const gl::Limitations &rendererLimitations,
                        GLenum type);
    void deleteShader(GLuint shader);
    Shader *getShader(GLuint handle) const;

    GLuint createProgram(rx::GLImplFactory *factory);
    void deleteProgram(GLuint program);
    Program *getProgram(GLuint handle) const;

  protected:
    ~ShaderProgramManager() override;

  private:
    template <typename ObjectType>
    void deleteObject(ResourceMap<ObjectType> *objectMap, GLuint id);

    ResourceMap<Shader> mShaders;
    ResourceMap<Program> mPrograms;
};

class TextureManager : public TypedResourceManager<Texture, HandleAllocator>
{
  public:
    GLuint createTexture();
    void deleteTexture(GLuint texture);
    Texture *getTexture(GLuint handle) const;
    Texture *checkTextureAllocation(rx::GLImplFactory *factory, GLuint handle, GLenum type);
    bool isTextureGenerated(GLuint texture) const;

  protected:
    ~TextureManager() override {}
};

class RenderbufferManager : public TypedResourceManager<Renderbuffer, HandleAllocator>
{
  public:
    GLuint createRenderbuffer();
    void deleteRenderbuffer(GLuint renderbuffer);
    Renderbuffer *getRenderbuffer(GLuint handle);
    Renderbuffer *checkRenderbufferAllocation(rx::GLImplFactory *factory, GLuint handle);
    bool isRenderbufferGenerated(GLuint renderbuffer) const;

  protected:
    ~RenderbufferManager() override {}
};

class SamplerManager : public TypedResourceManager<Sampler, HandleAllocator>
{
  public:
    GLuint createSampler();
    void deleteSampler(GLuint sampler);
    Sampler *getSampler(GLuint handle);
    Sampler *checkSamplerAllocation(rx::GLImplFactory *factory, GLuint handle);
    bool isSampler(GLuint sampler);

  protected:
    ~SamplerManager() override {}
};

class FenceSyncManager : public TypedResourceManager<FenceSync, HandleAllocator>
{
  public:
    GLuint createFenceSync(rx::GLImplFactory *factory);
    void deleteFenceSync(GLuint fenceSync);
    FenceSync *getFenceSync(GLuint handle);

  protected:
    ~FenceSyncManager() override {}
};

class PathManager : public ResourceManagerBase<HandleRangeAllocator>
{
  public:
    ErrorOrResult<GLuint> createPaths(rx::GLImplFactory *factory, GLsizei range);
    void deletePaths(GLuint first, GLsizei range);
    Path *getPath(GLuint handle) const;
    bool hasPath(GLuint handle) const;

  protected:
    ~PathManager() override;

  private:
    ResourceMap<Path> mPaths;
};

}  // namespace gl

#endif // LIBANGLE_RESOURCEMANAGER_H_
