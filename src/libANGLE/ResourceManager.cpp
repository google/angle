//
// Copyright (c) 2002-2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ResourceManager.cpp: Implements the the ResourceManager classes, which handle allocation and
// lifetime of GL objects.

#include "libANGLE/ResourceManager.h"

#include "libANGLE/Buffer.h"
#include "libANGLE/Fence.h"
#include "libANGLE/Path.h"
#include "libANGLE/Program.h"
#include "libANGLE/Renderbuffer.h"
#include "libANGLE/Sampler.h"
#include "libANGLE/Shader.h"
#include "libANGLE/Texture.h"
#include "libANGLE/renderer/GLImplFactory.h"

namespace gl
{

namespace
{

template <typename ResourceType>
GLuint AllocateEmptyObject(HandleAllocator *handleAllocator, ResourceMap<ResourceType> *objectMap)
{
    GLuint handle        = handleAllocator->allocate();
    (*objectMap)[handle] = nullptr;
    return handle;
}

template <typename ResourceType, typename CreationFunction>
GLuint InsertObject(HandleAllocator *handleAllocator,
                    ResourceMap<ResourceType> *objectMap,
                    const CreationFunction &func)
{
    GLuint handle        = handleAllocator->allocate();
    (*objectMap)[handle] = func(handle);
    return handle;
}

template <typename ResourceType, typename DeletionFunction>
void DeleteObject(HandleAllocator *handleAllocator,
                  ResourceMap<ResourceType> *objectMap,
                  GLuint handle,
                  const DeletionFunction &deletionFunc)
{
    auto objectIter = objectMap->find(handle);
    if (objectIter == objectMap->end())
    {
        return;
    }

    if (objectIter->second != nullptr)
    {
        deletionFunc(objectIter->second);
    }

    handleAllocator->release(objectIter->first);
    objectMap->erase(objectIter);
}

template <typename ResourceType>
ResourceType *GetObject(const ResourceMap<ResourceType> &objectMap, GLuint handle)
{
    auto iter = objectMap.find(handle);
    return iter != objectMap.end() ? iter->second : nullptr;
}

}  // anonymous namespace

template <typename HandleAllocatorType>
ResourceManagerBase<HandleAllocatorType>::ResourceManagerBase() : mRefCount(1)
{
}

template <typename HandleAllocatorType>
void ResourceManagerBase<HandleAllocatorType>::addRef()
{
    mRefCount++;
}

template <typename HandleAllocatorType>
void ResourceManagerBase<HandleAllocatorType>::release()
{
    if (--mRefCount == 0)
    {
        delete this;
    }
}

template <typename ResourceType, typename HandleAllocatorType>
TypedResourceManager<ResourceType, HandleAllocatorType>::~TypedResourceManager()
{
    while (!mObjectMap.empty())
    {
        deleteObject(mObjectMap.begin()->first);
    }
}

template <typename ResourceType, typename HandleAllocatorType>
void TypedResourceManager<ResourceType, HandleAllocatorType>::deleteObject(GLuint handle)
{
    DeleteObject(&this->mHandleAllocator, &mObjectMap, handle,
                 [](ResourceType *object) { object->release(); });
}

// Instantiations of ResourceManager
template class ResourceManagerBase<HandleAllocator>;
template class ResourceManagerBase<HandleRangeAllocator>;
template class TypedResourceManager<Buffer, HandleAllocator>;
template class TypedResourceManager<Texture, HandleAllocator>;
template class TypedResourceManager<Renderbuffer, HandleAllocator>;
template class TypedResourceManager<Sampler, HandleAllocator>;
template class TypedResourceManager<FenceSync, HandleAllocator>;

GLuint BufferManager::createBuffer()
{
    return AllocateEmptyObject(&mHandleAllocator, &mObjectMap);
}

void BufferManager::deleteBuffer(GLuint buffer)
{
    deleteObject(buffer);
}

Buffer *BufferManager::getBuffer(GLuint handle) const
{
    return GetObject(mObjectMap, handle);
}

Buffer *BufferManager::checkBufferAllocation(rx::GLImplFactory *factory, GLuint handle)
{
    if (handle == 0)
    {
        return nullptr;
    }

    auto objectMapIter   = mObjectMap.find(handle);
    bool handleAllocated = (objectMapIter != mObjectMap.end());

    if (handleAllocated && objectMapIter->second != nullptr)
    {
        return objectMapIter->second;
    }

    Buffer *object = new Buffer(factory, handle);
    object->addRef();

    if (handleAllocated)
    {
        objectMapIter->second = object;
    }
    else
    {
        mHandleAllocator.reserve(handle);
        mObjectMap[handle] = object;
    }

    return object;
}

bool BufferManager::isBufferGenerated(GLuint buffer) const
{
    return buffer == 0 || mObjectMap.find(buffer) != mObjectMap.end();
}

ShaderProgramManager::~ShaderProgramManager()
{
    while (!mPrograms.empty())
    {
        deleteProgram(mPrograms.begin()->first);
    }
    while (!mShaders.empty())
    {
        deleteShader(mShaders.begin()->first);
    }
}

GLuint ShaderProgramManager::createShader(rx::GLImplFactory *factory,
                                          const gl::Limitations &rendererLimitations,
                                          GLenum type)
{
    ASSERT(type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER || type == GL_COMPUTE_SHADER);
    GLuint handle    = mHandleAllocator.allocate();
    mShaders[handle] = new Shader(this, factory, rendererLimitations, type, handle);
    return handle;
}

void ShaderProgramManager::deleteShader(GLuint shader)
{
    deleteObject(&mShaders, shader);
}

Shader *ShaderProgramManager::getShader(GLuint handle) const
{
    return GetObject(mShaders, handle);
}

GLuint ShaderProgramManager::createProgram(rx::GLImplFactory *factory)
{
    GLuint handle     = mHandleAllocator.allocate();
    mPrograms[handle] = new Program(factory, this, handle);
    return handle;
}

void ShaderProgramManager::deleteProgram(GLuint program)
{
    deleteObject(&mPrograms, program);
}

Program *ShaderProgramManager::getProgram(GLuint handle) const
{
    return GetObject(mPrograms, handle);
}

template <typename ObjectType>
void ShaderProgramManager::deleteObject(ResourceMap<ObjectType> *objectMap, GLuint id)
{
    auto iter = objectMap->find(id);
    if (iter == objectMap->end())
    {
        return;
    }

    auto object = iter->second;
    if (object->getRefCount() == 0)
    {
        mHandleAllocator.release(id);
        SafeDelete(object);
        objectMap->erase(iter);
    }
    else
    {
        object->flagForDeletion();
    }
}

GLuint TextureManager::createTexture()
{
    return AllocateEmptyObject(&mHandleAllocator, &mObjectMap);
}

void TextureManager::deleteTexture(GLuint texture)
{
    deleteObject(texture);
}

Texture *TextureManager::getTexture(GLuint handle) const
{
    ASSERT(GetObject(mObjectMap, 0) == nullptr);
    return GetObject(mObjectMap, handle);
}

Texture *TextureManager::checkTextureAllocation(rx::GLImplFactory *factory,
                                                GLuint handle,
                                                GLenum type)
{
    if (handle == 0)
    {
        return nullptr;
    }

    auto objectMapIter   = mObjectMap.find(handle);
    bool handleAllocated = (objectMapIter != mObjectMap.end());

    if (handleAllocated && objectMapIter->second != nullptr)
    {
        return objectMapIter->second;
    }

    Texture *object = new Texture(factory, handle, type);
    object->addRef();

    if (handleAllocated)
    {
        objectMapIter->second = object;
    }
    else
    {
        mHandleAllocator.reserve(handle);
        mObjectMap[handle] = object;
    }

    return object;
}

bool TextureManager::isTextureGenerated(GLuint texture) const
{
    return texture == 0 || mObjectMap.find(texture) != mObjectMap.end();
}

GLuint RenderbufferManager::createRenderbuffer()
{
    return AllocateEmptyObject(&mHandleAllocator, &mObjectMap);
}

void RenderbufferManager::deleteRenderbuffer(GLuint renderbuffer)
{
    deleteObject(renderbuffer);
}

Renderbuffer *RenderbufferManager::getRenderbuffer(GLuint handle)
{
    return GetObject(mObjectMap, handle);
}

Renderbuffer *RenderbufferManager::checkRenderbufferAllocation(rx::GLImplFactory *factory,
                                                               GLuint handle)
{
    if (handle == 0)
    {
        return nullptr;
    }

    auto objectMapIter   = mObjectMap.find(handle);
    bool handleAllocated = (objectMapIter != mObjectMap.end());

    if (handleAllocated && objectMapIter->second != nullptr)
    {
        return objectMapIter->second;
    }

    Renderbuffer *object = new Renderbuffer(factory->createRenderbuffer(), handle);
    object->addRef();

    if (handleAllocated)
    {
        objectMapIter->second = object;
    }
    else
    {
        mHandleAllocator.reserve(handle);
        mObjectMap[handle] = object;
    }

    return object;
}

bool RenderbufferManager::isRenderbufferGenerated(GLuint renderbuffer) const
{
    return renderbuffer == 0 || mObjectMap.find(renderbuffer) != mObjectMap.end();
}

GLuint SamplerManager::createSampler()
{
    return AllocateEmptyObject(&mHandleAllocator, &mObjectMap);
}

void SamplerManager::deleteSampler(GLuint sampler)
{
    deleteObject(sampler);
}

Sampler *SamplerManager::getSampler(GLuint handle)
{
    return GetObject(mObjectMap, handle);
}

Sampler *SamplerManager::checkSamplerAllocation(rx::GLImplFactory *factory, GLuint handle)
{
    if (handle == 0)
    {
        return nullptr;
    }

    auto objectMapIter   = mObjectMap.find(handle);
    bool handleAllocated = (objectMapIter != mObjectMap.end());

    if (handleAllocated && objectMapIter->second != nullptr)
    {
        return objectMapIter->second;
    }

    Sampler *object = new Sampler(factory, handle);
    object->addRef();

    if (handleAllocated)
    {
        objectMapIter->second = object;
    }
    else
    {
        mHandleAllocator.reserve(handle);
        mObjectMap[handle] = object;
    }

    return object;
}

bool SamplerManager::isSampler(GLuint sampler)
{
    return mObjectMap.find(sampler) != mObjectMap.end();
}

GLuint FenceSyncManager::createFenceSync(rx::GLImplFactory *factory)
{
    struct fenceSyncAllocator
    {
        fenceSyncAllocator(rx::GLImplFactory *factory) : factory(factory) {}

        rx::GLImplFactory *factory;

        FenceSync *operator()(GLuint handle) const
        {
            FenceSync *fenceSync = new FenceSync(factory->createFenceSync(), handle);
            fenceSync->addRef();
            return fenceSync;
        }
    };

    return InsertObject(&mHandleAllocator, &mObjectMap, fenceSyncAllocator(factory));
}

void FenceSyncManager::deleteFenceSync(GLuint fenceSync)
{
    deleteObject(fenceSync);
}

FenceSync *FenceSyncManager::getFenceSync(GLuint handle)
{
    return GetObject(mObjectMap, handle);
}

ErrorOrResult<GLuint> PathManager::createPaths(rx::GLImplFactory *factory, GLsizei range)
{
    // Allocate client side handles.
    const GLuint client = mHandleAllocator.allocateRange(static_cast<GLuint>(range));
    if (client == HandleRangeAllocator::kInvalidHandle)
        return Error(GL_OUT_OF_MEMORY, "Failed to allocate path handle range.");

    const auto &paths = factory->createPaths(range);
    if (paths.empty())
    {
        mHandleAllocator.releaseRange(client, range);
        return Error(GL_OUT_OF_MEMORY, "Failed to allocate path objects.");
    }

    auto hint = mPaths.begin();

    for (GLsizei i = 0; i < range; ++i)
    {
        const auto impl = paths[static_cast<unsigned>(i)];
        const auto id   = client + i;
        hint            = mPaths.insert(hint, std::make_pair(id, new Path(impl)));
    }
    return client;
}

void PathManager::deletePaths(GLuint first, GLsizei range)
{
    for (GLsizei i = 0; i < range; ++i)
    {
        const auto id = first + i;
        const auto it = mPaths.find(id);
        if (it == mPaths.end())
            continue;
        Path *p = it->second;
        delete p;
        mPaths.erase(it);
    }
    mHandleAllocator.releaseRange(first, static_cast<GLuint>(range));
}

Path *PathManager::getPath(GLuint handle) const
{
    auto iter = mPaths.find(handle);
    return iter != mPaths.end() ? iter->second : nullptr;
}

bool PathManager::hasPath(GLuint handle) const
{
    return mHandleAllocator.isUsed(handle);
}

PathManager::~PathManager()
{
    for (auto path : mPaths)
    {
        SafeDelete(path.second);
    }
}

}  // namespace gl
