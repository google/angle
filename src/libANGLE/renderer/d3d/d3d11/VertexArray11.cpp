//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// VertexArray11:
//   Implementation of rx::VertexArray11.
//

#include "libANGLE/renderer/d3d/d3d11/VertexArray11.h"

#include "common/bitset_utils.h"
#include "libANGLE/Context.h"
#include "libANGLE/renderer/d3d/d3d11/Buffer11.h"
#include "libANGLE/renderer/d3d/d3d11/Context11.h"

using namespace angle;

namespace rx
{
VertexArray11::VertexArray11(const gl::VertexArrayState &data)
    : VertexArrayImpl(data),
      mAttributeStorageTypes(data.getMaxAttribs(), VertexStorageType::CURRENT_VALUE),
      mTranslatedAttribs(data.getMaxAttribs()),
      mCurrentArrayBuffers(data.getMaxAttribs()),
      mCurrentElementArrayBuffer(),
      mOnArrayBufferDataDirty(),
      mOnElementArrayBufferDataDirty(this, mCurrentArrayBuffers.size()),
      mAppliedNumViewsToDivisor(1),
      mLastElementType(GL_NONE),
      mLastDrawElementsOffset(0),
      mCurrentElementArrayStorage(IndexStorageType::Invalid),
      mCachedIndexInfo{0},
      mCachedIndexInfoValid(false)
{
    for (size_t attribIndex = 0; attribIndex < mCurrentArrayBuffers.size(); ++attribIndex)
    {
        mOnArrayBufferDataDirty.emplace_back(this, attribIndex);
    }
}

VertexArray11::~VertexArray11()
{
}

void VertexArray11::destroy(const gl::Context *context)
{
    for (auto &buffer : mCurrentArrayBuffers)
    {
        if (buffer.get())
        {
            buffer.set(context, nullptr);
        }
    }

    mCurrentElementArrayBuffer.set(context, nullptr);
}

gl::Error VertexArray11::syncState(const gl::Context *context,
                                   const gl::VertexArray::DirtyBits &dirtyBits,
                                   const gl::VertexArray::DirtyAttribBitsArray &attribBits,
                                   const gl::VertexArray::DirtyBindingBitsArray &bindingBits)
{
    ASSERT(dirtyBits.any());

    // Generate a state serial. This serial is used in the program class to validate the cached
    // input layout, and skip recomputation in the fast path.
    Renderer11 *renderer = GetImplAs<Context11>(context)->getRenderer();
    mCurrentStateSerial = renderer->generateSerial();

    // TODO(jmadill): Individual attribute invalidation.
    renderer->getStateManager()->invalidateVertexBuffer();

    for (auto dirtyBit : dirtyBits)
    {
        if (dirtyBit == gl::VertexArray::DIRTY_BIT_ELEMENT_ARRAY_BUFFER)
        {
            mCachedIndexInfoValid = false;
            mLastElementType      = GL_NONE;
        }
        else
        {
            size_t index = gl::VertexArray::GetVertexIndexFromDirtyBit(dirtyBit);
            // TODO(jiawei.shao@intel.com): Vertex Attrib Bindings
            ASSERT(index == mState.getBindingIndexFromAttribIndex(index));
            mAttribsToUpdate.set(index);
        }
    }

    return gl::NoError();
}

bool VertexArray11::flushAttribUpdates(const gl::Context *context)
{
    if (mAttribsToUpdate.any())
    {
        const auto &activeLocations =
            context->getGLState().getProgram()->getActiveAttribLocationsMask();

        // Skip attrib locations the program doesn't use.
        gl::AttributesMask activeToUpdate = mAttribsToUpdate & activeLocations;

        for (auto toUpdateIndex : activeToUpdate)
        {
            mAttribsToUpdate.reset(toUpdateIndex);
            updateVertexAttribStorage(context, toUpdateIndex);
        }

        return true;
    }

    return false;
}

bool VertexArray11::updateElementArrayStorage(const gl::Context *context,
                                              GLenum elementType,
                                              GLenum destElementType,
                                              const void *indices)
{
    unsigned int offset = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(indices));

    if (mCachedIndexInfoValid && mLastElementType == elementType &&
        offset == mLastDrawElementsOffset)
    {
        // Dynamic index buffers must be re-streamed every draw.
        return (mCurrentElementArrayStorage == IndexStorageType::Dynamic);
    }

    gl::Buffer *newBuffer           = mState.getElementArrayBuffer().get();
    gl::Buffer *oldBuffer           = mCurrentElementArrayBuffer.get();
    bool needsTranslation           = false;
    IndexStorageType newStorageType = ClassifyIndexStorage(
        context->getGLState(), newBuffer, elementType, destElementType, offset, &needsTranslation);

    if (newBuffer != oldBuffer)
    {
        mCurrentElementArrayBuffer.set(context, newBuffer);
    }

    if (newStorageType != mCurrentElementArrayStorage || newBuffer != oldBuffer)
    {
        Buffer11 *newBuffer11 = SafeGetImplAs<Buffer11>(newBuffer);

        mCurrentElementArrayStorage = newStorageType;
        mOnElementArrayBufferDataDirty.bind(newBuffer11);
        needsTranslation = true;
    }

    if (mLastDrawElementsOffset != offset)
    {
        needsTranslation        = true;
        mLastDrawElementsOffset = offset;
    }

    if (mLastElementType != elementType)
    {
        needsTranslation = true;
        mLastElementType = elementType;
    }

    // TODO(jmadill): We should probably promote static usage immediately, because this can change
    // the storage type for dynamic buffers.
    return needsTranslation || !mCachedIndexInfoValid;
}

void VertexArray11::updateVertexAttribStorage(const gl::Context *context, size_t attribIndex)
{
    const auto &attrib  = mState.getVertexAttribute(attribIndex);
    const auto &binding = mState.getBindingFromAttribIndex(attribIndex);

    // Note: having an unchanged storage type doesn't mean the attribute is clean.
    auto oldStorageType = mAttributeStorageTypes[attribIndex];
    auto newStorageType = ClassifyAttributeStorage(attrib, binding);

    mAttributeStorageTypes[attribIndex] = newStorageType;

    StateManager11 *stateManager = GetImplAs<Context11>(context)->getRenderer()->getStateManager();

    if (newStorageType == VertexStorageType::DYNAMIC)
    {
        if (oldStorageType != VertexStorageType::DYNAMIC)
        {
            // Sync dynamic attribs in a different set.
            mAttribsToTranslate.reset(attribIndex);
            mDynamicAttribsMask.set(attribIndex);
        }
    }
    else
    {
        mAttribsToTranslate.set(attribIndex);

        if (oldStorageType == VertexStorageType::DYNAMIC)
        {
            ASSERT(mDynamicAttribsMask[attribIndex]);
            mDynamicAttribsMask.reset(attribIndex);
        }
    }
    stateManager->invalidateVertexAttributeTranslation();

    gl::Buffer *oldBufferGL = mCurrentArrayBuffers[attribIndex].get();
    gl::Buffer *newBufferGL = attrib.enabled ? binding.getBuffer().get() : nullptr;
    Buffer11 *oldBuffer11   = oldBufferGL ? GetImplAs<Buffer11>(oldBufferGL) : nullptr;
    Buffer11 *newBuffer11   = newBufferGL ? GetImplAs<Buffer11>(newBufferGL) : nullptr;

    if (oldBuffer11 != newBuffer11 || oldStorageType != newStorageType)
    {
        if (newStorageType == VertexStorageType::CURRENT_VALUE)
        {
            stateManager->invalidateCurrentValueAttrib(attribIndex);
        }

        mOnArrayBufferDataDirty[attribIndex].bind(newBuffer11);
        mCurrentArrayBuffers[attribIndex].set(context, binding.getBuffer().get());
    }
}

bool VertexArray11::hasActiveDynamicAttrib(const gl::Context *context)
{
    flushAttribUpdates(context);
    const auto &activeLocations =
        context->getGLState().getProgram()->getActiveAttribLocationsMask();
    auto activeDynamicAttribs = (mDynamicAttribsMask & activeLocations);
    return activeDynamicAttribs.any();
}

gl::Error VertexArray11::updateDirtyAndDynamicAttribs(const gl::Context *context,
                                                      VertexDataManager *vertexDataManager,
                                                      const gl::DrawCallParams &drawCallParams)
{
    flushAttribUpdates(context);

    const auto &glState         = context->getGLState();
    const gl::Program *program  = glState.getProgram();
    const auto &activeLocations = program->getActiveAttribLocationsMask();
    const auto &attribs         = mState.getVertexAttributes();
    const auto &bindings        = mState.getVertexBindings();
    mAppliedNumViewsToDivisor =
        (program != nullptr && program->usesMultiview()) ? program->getNumViews() : 1;

    if (mAttribsToTranslate.any())
    {
        // Skip attrib locations the program doesn't use, saving for the next frame.
        gl::AttributesMask dirtyActiveAttribs = (mAttribsToTranslate & activeLocations);

        for (auto dirtyAttribIndex : dirtyActiveAttribs)
        {
            mAttribsToTranslate.reset(dirtyAttribIndex);

            auto *translatedAttrib = &mTranslatedAttribs[dirtyAttribIndex];
            const auto &currentValue = glState.getVertexAttribCurrentValue(dirtyAttribIndex);

            // Record basic attrib info
            translatedAttrib->attribute = &attribs[dirtyAttribIndex];
            translatedAttrib->binding   = &bindings[translatedAttrib->attribute->bindingIndex];
            translatedAttrib->currentValueType = currentValue.Type;
            translatedAttrib->divisor =
                translatedAttrib->binding->getDivisor() * mAppliedNumViewsToDivisor;

            switch (mAttributeStorageTypes[dirtyAttribIndex])
            {
                case VertexStorageType::DIRECT:
                    VertexDataManager::StoreDirectAttrib(translatedAttrib);
                    break;
                case VertexStorageType::STATIC:
                {
                    ANGLE_TRY(VertexDataManager::StoreStaticAttrib(context, translatedAttrib));
                    break;
                }
                case VertexStorageType::CURRENT_VALUE:
                    // Current value attribs are managed by the StateManager11.
                    break;
                default:
                    UNREACHABLE();
                    break;
            }
        }
    }

    if (mDynamicAttribsMask.any())
    {
        ANGLE_TRY(drawCallParams.ensureIndexRangeResolved(context));

        auto activeDynamicAttribs = (mDynamicAttribsMask & activeLocations);
        if (activeDynamicAttribs.none())
        {
            return gl::NoError();
        }

        for (auto dynamicAttribIndex : activeDynamicAttribs)
        {
            auto *dynamicAttrib = &mTranslatedAttribs[dynamicAttribIndex];
            const auto &currentValue = glState.getVertexAttribCurrentValue(dynamicAttribIndex);

            // Record basic attrib info
            dynamicAttrib->attribute        = &attribs[dynamicAttribIndex];
            dynamicAttrib->binding          = &bindings[dynamicAttrib->attribute->bindingIndex];
            dynamicAttrib->currentValueType = currentValue.Type;
            dynamicAttrib->divisor =
                dynamicAttrib->binding->getDivisor() * mAppliedNumViewsToDivisor;
        }

        ANGLE_TRY(vertexDataManager->storeDynamicAttribs(
            context, &mTranslatedAttribs, activeDynamicAttribs, drawCallParams.firstVertex(),
            drawCallParams.vertexCount(), drawCallParams.instances()));
    }

    return gl::NoError();
}

const std::vector<TranslatedAttribute> &VertexArray11::getTranslatedAttribs() const
{
    return mTranslatedAttribs;
}

void VertexArray11::onSubjectStateChange(const gl::Context *context,
                                         angle::SubjectIndex index,
                                         angle::SubjectMessage message)
{
    if (index == mAttributeStorageTypes.size())
    {
        mCachedIndexInfoValid   = false;
        mLastElementType        = GL_NONE;
        mLastDrawElementsOffset = 0;
    }
    else
    {
        ASSERT(mAttributeStorageTypes[index] != VertexStorageType::CURRENT_VALUE);

        // This can change a buffer's storage, we'll need to re-check.
        if (mAttributeStorageTypes[index] != VertexStorageType::DIRECT ||
            message != angle::SubjectMessage::CONTENTS_CHANGED)
        {
            mAttribsToUpdate.set(index);

            // Changing the vertex attribute state can affect the vertex shader.
            Renderer11 *renderer = GetImplAs<Context11>(context)->getRenderer();
            renderer->getStateManager()->invalidateShaders();
        }
    }
}

void VertexArray11::clearDirtyAndPromoteDynamicAttribs(const gl::Context *context,
                                                       const gl::DrawCallParams &drawCallParams)
{
    const gl::State &state      = context->getGLState();
    const gl::Program *program  = state.getProgram();
    const auto &activeLocations = program->getActiveAttribLocationsMask();
    mAttribsToUpdate &= ~activeLocations;

    // Promote to static after we clear the dirty attributes, otherwise we can lose dirtyness.
    auto activeDynamicAttribs = (mDynamicAttribsMask & activeLocations);
    if (activeDynamicAttribs.any())
    {
        VertexDataManager::PromoteDynamicAttribs(context, mTranslatedAttribs, activeDynamicAttribs,
                                                 drawCallParams.vertexCount());
    }
}

void VertexArray11::markAllAttributeDivisorsForAdjustment(int numViews)
{
    if (mAppliedNumViewsToDivisor != numViews)
    {
        mAppliedNumViewsToDivisor = numViews;
        mAttribsToUpdate.set();
    }
}

TranslatedIndexData *VertexArray11::getCachedIndexInfo()
{
    return &mCachedIndexInfo;
}

void VertexArray11::setCachedIndexInfoValid()
{
    mCachedIndexInfoValid = true;
}

bool VertexArray11::isCachedIndexInfoValid() const
{
    return mCachedIndexInfoValid;
}

}  // namespace rx
