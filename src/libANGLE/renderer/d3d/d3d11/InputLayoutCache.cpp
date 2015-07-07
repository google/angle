//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// InputLayoutCache.cpp: Defines InputLayoutCache, a class that builds and caches
// D3D11 input layouts.

#include "libANGLE/renderer/d3d/d3d11/InputLayoutCache.h"
#include "libANGLE/renderer/d3d/d3d11/VertexBuffer11.h"
#include "libANGLE/renderer/d3d/d3d11/Buffer11.h"
#include "libANGLE/renderer/d3d/d3d11/ShaderExecutable11.h"
#include "libANGLE/renderer/d3d/d3d11/formatutils11.h"
#include "libANGLE/renderer/d3d/ProgramD3D.h"
#include "libANGLE/renderer/d3d/VertexDataManager.h"
#include "libANGLE/renderer/d3d/IndexDataManager.h"
#include "libANGLE/Program.h"
#include "libANGLE/VertexAttribute.h"

#include "third_party/murmurhash/MurmurHash3.h"

namespace rx
{

namespace
{

void GetInputLayout(const TranslatedAttribute *translatedAttributes[gl::MAX_VERTEX_ATTRIBS],
                    size_t attributeCount,
                    gl::InputLayout *inputLayout)
{
    for (size_t attributeIndex = 0; attributeIndex < attributeCount; ++attributeIndex)
    {
        const TranslatedAttribute *translatedAttribute = translatedAttributes[attributeIndex];

        if (translatedAttribute->active)
        {
            gl::VertexFormatType vertexFormatType =
                gl::GetVertexFormatType(*translatedAttribute->attribute,
                translatedAttribute->currentValueType);
            inputLayout->push_back(vertexFormatType);
        }
        else
        {
            inputLayout->push_back(gl::VERTEX_FORMAT_INVALID);
        }
    }
}

const unsigned int kDefaultCacheSize = 1024;

} // anonymous namespace

bool InputLayoutCache::PackedAttributeComparator::operator()(const PackedAttributeLayout &a,
                                                             const PackedAttributeLayout &b) const
{
    if (a.numAttributes != b.numAttributes)
    {
        return a.numAttributes < b.numAttributes;
    }

    if (a.flags != b.flags)
    {
        return a.flags < b.flags;
    }

    for (size_t attribIndex = 0; attribIndex < a.numAttributes; attribIndex++)
    {
        const auto &attribA = a.attributeData[attribIndex];
        const auto &attribB = b.attributeData[attribIndex];

        if (attribA.glType != attribB.glType)
            return attribA.glType < attribB.glType;
        if (attribA.semanticIndex != attribB.semanticIndex)
            return attribA.semanticIndex < attribB.semanticIndex;
        if (attribA.dxgiFormat != attribB.dxgiFormat)
            return attribA.dxgiFormat < attribB.dxgiFormat;
        if (attribA.divisor != attribB.divisor)
            return attribA.divisor < attribB.divisor;
    }

    // Equal
    return false;
}

InputLayoutCache::InputLayoutCache()
    : mCacheSize(kDefaultCacheSize)
{
    mCounter = 0;
    mDevice = NULL;
    mDeviceContext = NULL;
    mCurrentIL = NULL;
    for (unsigned int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        mCurrentBuffers[i] = NULL;
        mCurrentVertexStrides[i] = static_cast<UINT>(-1);
        mCurrentVertexOffsets[i] = static_cast<UINT>(-1);
    }
    mPointSpriteVertexBuffer = NULL;
    mPointSpriteIndexBuffer = NULL;
}

InputLayoutCache::~InputLayoutCache()
{
    clear();
}

void InputLayoutCache::initialize(ID3D11Device *device, ID3D11DeviceContext *context)
{
    clear();
    mDevice = device;
    mDeviceContext = context;
    mFeatureLevel = device->GetFeatureLevel();
}

void InputLayoutCache::clear()
{
    for (auto &layout : mLayoutMap)
    {
        SafeRelease(layout.second);
    }
    mLayoutMap.clear();
    SafeRelease(mPointSpriteVertexBuffer);
    SafeRelease(mPointSpriteIndexBuffer);
    markDirty();
}

void InputLayoutCache::markDirty()
{
    mCurrentIL = NULL;
    for (unsigned int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        mCurrentBuffers[i] = NULL;
        mCurrentVertexStrides[i] = static_cast<UINT>(-1);
        mCurrentVertexOffsets[i] = static_cast<UINT>(-1);
    }
}

gl::Error InputLayoutCache::applyVertexBuffers(const std::vector<TranslatedAttribute> &unsortedAttributes,
                                               GLenum mode, gl::Program *program, SourceIndexData *sourceInfo)
{
    ProgramD3D *programD3D = GetImplAs<ProgramD3D>(program);

    int sortedSemanticIndices[gl::MAX_VERTEX_ATTRIBS];
    const TranslatedAttribute *sortedAttributes[gl::MAX_VERTEX_ATTRIBS] = { nullptr };
    programD3D->sortAttributesByLayout(unsortedAttributes, sortedSemanticIndices, sortedAttributes);
    bool programUsesInstancedPointSprites = programD3D->usesPointSize() && programD3D->usesInstancedPointSpriteEmulation();
    bool instancedPointSpritesActive = programUsesInstancedPointSprites && (mode == GL_POINTS);
    bool indexedPointSpriteEmulationActive = instancedPointSpritesActive && (sourceInfo != nullptr);

    if (!mDevice || !mDeviceContext)
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Internal input layout cache is not initialized.");
    }

    InputLayoutKey ilKey;
    ilKey.elementCount = 0;

    PackedAttributeLayout layout;

    static const char* semanticName = "TEXCOORD";

    unsigned int firstIndexedElement = gl::MAX_VERTEX_ATTRIBS;
    unsigned int firstInstancedElement = gl::MAX_VERTEX_ATTRIBS;
    unsigned int nextAvailableInputSlot = 0;

    for (unsigned int i = 0; i < unsortedAttributes.size(); i++)
    {
        if (sortedAttributes[i]->active)
        {
            D3D11_INPUT_CLASSIFICATION inputClass = sortedAttributes[i]->divisor > 0 ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
            // If rendering points and instanced pointsprite emulation is being used, the inputClass is required to be configured as per instance data
            inputClass = instancedPointSpritesActive ? D3D11_INPUT_PER_INSTANCE_DATA : inputClass;

            gl::VertexFormatType vertexFormatType = gl::GetVertexFormatType(*sortedAttributes[i]->attribute, sortedAttributes[i]->currentValueType);
            const d3d11::VertexFormat &vertexFormatInfo = d3d11::GetVertexFormatInfo(vertexFormatType, mFeatureLevel);

            // Record the type of the associated vertex shader vector in our key
            // This will prevent mismatched vertex shaders from using the same input layout
            GLint attributeSize;
            program->getActiveAttribute(ilKey.elementCount, 0, NULL, &attributeSize, &ilKey.elements[ilKey.elementCount].glslElementType, NULL);

            ilKey.elements[ilKey.elementCount].desc.SemanticName = semanticName;
            ilKey.elements[ilKey.elementCount].desc.SemanticIndex = sortedSemanticIndices[i];
            ilKey.elements[ilKey.elementCount].desc.Format = vertexFormatInfo.nativeFormat;
            ilKey.elements[ilKey.elementCount].desc.InputSlot = i;
            ilKey.elements[ilKey.elementCount].desc.AlignedByteOffset = 0;
            ilKey.elements[ilKey.elementCount].desc.InputSlotClass = inputClass;
            ilKey.elements[ilKey.elementCount].desc.InstanceDataStepRate = instancedPointSpritesActive ? 1 : sortedAttributes[i]->divisor;

            if (inputClass == D3D11_INPUT_PER_VERTEX_DATA && firstIndexedElement == gl::MAX_VERTEX_ATTRIBS)
            {
                firstIndexedElement = ilKey.elementCount;
            }
            else if (inputClass == D3D11_INPUT_PER_INSTANCE_DATA && firstInstancedElement == gl::MAX_VERTEX_ATTRIBS)
            {
                firstInstancedElement = ilKey.elementCount;
            }

            ilKey.elementCount++;
            nextAvailableInputSlot = i + 1;

            layout.addAttributeData(ilKey.elements[ilKey.elementCount].glslElementType,
                                    sortedSemanticIndices[i],
                                    vertexFormatInfo.nativeFormat,
                                    sortedAttributes[i]->divisor);
        }
    }

    // Instanced PointSprite emulation requires additional entries in the
    // inputlayout to support the vertices that make up the pointsprite quad.
    // We do this even if mode != GL_POINTS, since the shader signature has these inputs, and the input layout must match the shader
    if (programUsesInstancedPointSprites)
    {
        ilKey.elements[ilKey.elementCount].desc.SemanticName = "SPRITEPOSITION";
        ilKey.elements[ilKey.elementCount].desc.SemanticIndex = 0;
        ilKey.elements[ilKey.elementCount].desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
        ilKey.elements[ilKey.elementCount].desc.InputSlot = nextAvailableInputSlot;
        ilKey.elements[ilKey.elementCount].desc.AlignedByteOffset = 0;
        ilKey.elements[ilKey.elementCount].desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        ilKey.elements[ilKey.elementCount].desc.InstanceDataStepRate = 0;

        // The new elements are D3D11_INPUT_PER_VERTEX_DATA data so the indexed element
        // tracking must be applied.  This ensures that the instancing specific
        // buffer swapping logic continues to work.
        if (firstIndexedElement == gl::MAX_VERTEX_ATTRIBS)
        {
            firstIndexedElement = ilKey.elementCount;
        }

        ilKey.elementCount++;

        ilKey.elements[ilKey.elementCount].desc.SemanticName = "SPRITETEXCOORD";
        ilKey.elements[ilKey.elementCount].desc.SemanticIndex = 0;
        ilKey.elements[ilKey.elementCount].desc.Format = DXGI_FORMAT_R32G32_FLOAT;
        ilKey.elements[ilKey.elementCount].desc.InputSlot = nextAvailableInputSlot;
        ilKey.elements[ilKey.elementCount].desc.AlignedByteOffset = sizeof(float) * 3;
        ilKey.elements[ilKey.elementCount].desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        ilKey.elements[ilKey.elementCount].desc.InstanceDataStepRate = 0;

        ilKey.elementCount++;
    }

    // On 9_3, we must ensure that slot 0 contains non-instanced data.
    // If slot 0 currently contains instanced data then we swap it with a non-instanced element.
    // Note that instancing is only available on 9_3 via ANGLE_instanced_arrays, since 9_3 doesn't support OpenGL ES 3.0.
    // As per the spec for ANGLE_instanced_arrays, not all attributes can be instanced simultaneously, so a non-instanced element must exist.
    ASSERT(!(mFeatureLevel <= D3D_FEATURE_LEVEL_9_3 && firstIndexedElement == gl::MAX_VERTEX_ATTRIBS));
    bool moveFirstIndexedIntoSlotZero = mFeatureLevel <= D3D_FEATURE_LEVEL_9_3 && firstInstancedElement == 0 && firstIndexedElement != gl::MAX_VERTEX_ATTRIBS;

    if (moveFirstIndexedIntoSlotZero)
    {
        ilKey.elements[firstInstancedElement].desc.InputSlot = ilKey.elements[firstIndexedElement].desc.InputSlot;
        ilKey.elements[firstIndexedElement].desc.InputSlot = 0;

        // Instanced PointSprite emulation uses multiple layout entries across a single vertex buffer.
        // If an index swap is performed, we need to ensure that all elements get the proper InputSlot.
        if (programUsesInstancedPointSprites)
        {
            ilKey.elements[firstIndexedElement + 1].desc.InputSlot = 0;
        }
    }

    if (programUsesInstancedPointSprites)
    {
        layout.flags |= PackedAttributeLayout::FLAG_USES_INSTANCED_SPRITES;
    }

    if (moveFirstIndexedIntoSlotZero)
    {
        layout.flags |= PackedAttributeLayout::FLAG_MOVE_FIRST_INDEXED;
    }

    if (instancedPointSpritesActive)
    {
        layout.flags |= PackedAttributeLayout::FLAG_INSTANCED_SPRITES_ACTIVE;
    }

    ID3D11InputLayout *inputLayout = nullptr;

    auto layoutMapIt = mLayoutMap.find(layout);
    if (layoutMapIt != mLayoutMap.end())
    {
        inputLayout = layoutMapIt->second;
    }
    else
    {
        gl::InputLayout shaderInputLayout;
        GetInputLayout(sortedAttributes, unsortedAttributes.size(), &shaderInputLayout);

        ShaderExecutableD3D *shader = nullptr;
        gl::Error error = programD3D->getVertexExecutableForInputLayout(shaderInputLayout, &shader, nullptr);
        if (error.isError())
        {
            return error;
        }

        ShaderExecutableD3D *shader11 = GetAs<ShaderExecutable11>(shader);

        D3D11_INPUT_ELEMENT_DESC descs[gl::MAX_VERTEX_ATTRIBS];
        for (unsigned int j = 0; j < ilKey.elementCount; ++j)
        {
            descs[j] = ilKey.elements[j].desc;
        }

        HRESULT result = mDevice->CreateInputLayout(descs, ilKey.elementCount, shader11->getFunction(), shader11->getLength(), &inputLayout);
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create internal input layout, HRESULT: 0x%08x", result);
        }

        if (mLayoutMap.size() >= mCacheSize)
        {
            TRACE("Overflowed the limit of %u input layouts, purging half the cache.", mCacheSize);

            // Randomly release every second element
            auto it = mLayoutMap.begin();
            while (it != mLayoutMap.end())
            {
                it++;
                if (it != mLayoutMap.end())
                {
                    // Calling std::map::erase invalidates the current iterator, so make a copy.
                    auto eraseIt = it++;
                    SafeRelease(eraseIt->second);
                    mLayoutMap.erase(eraseIt);
                }
            }
        }

        mLayoutMap[layout] = inputLayout;
    }

    if (inputLayout != mCurrentIL)
    {
        mDeviceContext->IASetInputLayout(inputLayout);
        mCurrentIL = inputLayout;
    }

    bool dirtyBuffers = false;
    size_t minDiff = gl::MAX_VERTEX_ATTRIBS;
    size_t maxDiff = 0;
    unsigned int nextAvailableIndex = 0;

    for (unsigned int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        ID3D11Buffer *buffer = NULL;
        UINT vertexStride = 0;
        UINT vertexOffset = 0;

        if (i < unsortedAttributes.size() && sortedAttributes[i]->active)
        {
            VertexBuffer11 *vertexBuffer = GetAs<VertexBuffer11>(sortedAttributes[i]->vertexBuffer);
            Buffer11 *bufferStorage = sortedAttributes[i]->storage ? GetAs<Buffer11>(sortedAttributes[i]->storage) : NULL;

            // If indexed pointsprite emulation is active, then we need to take a less efficent code path.
            // Emulated indexed pointsprite rendering requires that the vertex buffers match exactly to
            // the indices passed by the caller.  This could expand or shrink the vertex buffer depending
            // on the number of points indicated by the index list or how many duplicates are found on the index list.
            if (bufferStorage == nullptr)
            {
                buffer = vertexBuffer->getBuffer();
            }
            else if (indexedPointSpriteEmulationActive)
            {
                if (sourceInfo->srcBuffer != nullptr)
                {
                    const uint8_t *bufferData = nullptr;
                    gl::Error error = sourceInfo->srcBuffer->getData(&bufferData);
                    if (error.isError())
                    {
                        return error;
                    }
                    ASSERT(bufferData != nullptr);

                    ptrdiff_t offset = reinterpret_cast<ptrdiff_t>(sourceInfo->srcIndices);
                    sourceInfo->srcBuffer = nullptr;
                    sourceInfo->srcIndices = bufferData + offset;
                }

                buffer = bufferStorage->getEmulatedIndexedBuffer(sourceInfo, sortedAttributes[i]);
            }
            else
            {
                buffer = bufferStorage->getBuffer(BUFFER_USAGE_VERTEX_OR_TRANSFORM_FEEDBACK);
            }

            vertexStride = sortedAttributes[i]->stride;
            vertexOffset = sortedAttributes[i]->offset;
        }

        if (buffer != mCurrentBuffers[i] || vertexStride != mCurrentVertexStrides[i] ||
            vertexOffset != mCurrentVertexOffsets[i])
        {
            dirtyBuffers = true;
            minDiff = std::min(minDiff, static_cast<size_t>(i));
            maxDiff = std::max(maxDiff, static_cast<size_t>(i));

            mCurrentBuffers[i] = buffer;
            mCurrentVertexStrides[i] = vertexStride;
            mCurrentVertexOffsets[i] = vertexOffset;
        }

        // If a non null ID3D11Buffer is being assigned to mCurrentBuffers,
        // then the next available index needs to be tracked to ensure
        // that any instanced pointsprite emulation buffers will be properly packed.
        if (buffer)
        {
            nextAvailableIndex = i + 1;
        }
    }

    // Instanced PointSprite emulation requires two additional ID3D11Buffers.
    // A vertex buffer needs to be created and added to the list of current buffers,
    // strides and offsets collections.  This buffer contains the vertices for a single
    // PointSprite quad.
    // An index buffer also needs to be created and applied because rendering instanced
    // data on D3D11 FL9_3 requires DrawIndexedInstanced() to be used.
    if (instancedPointSpritesActive)
    {
        HRESULT result = S_OK;
        const UINT pointSpriteVertexStride = sizeof(float) * 5;

        if (!mPointSpriteVertexBuffer)
        {
            static const float pointSpriteVertices[] =
            {
                // Position        // TexCoord
               -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
               -1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
               -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
            };

            D3D11_SUBRESOURCE_DATA vertexBufferData = { pointSpriteVertices, 0, 0 };
            D3D11_BUFFER_DESC vertexBufferDesc;
            vertexBufferDesc.ByteWidth = sizeof(pointSpriteVertices);
            vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
            vertexBufferDesc.CPUAccessFlags = 0;
            vertexBufferDesc.MiscFlags = 0;
            vertexBufferDesc.StructureByteStride = 0;

            result = mDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &mPointSpriteVertexBuffer);
            if (FAILED(result))
            {
                return gl::Error(GL_OUT_OF_MEMORY, "Failed to create instanced pointsprite emulation vertex buffer, HRESULT: 0x%08x", result);
            }
        }

        mCurrentBuffers[nextAvailableIndex] = mPointSpriteVertexBuffer;
        mCurrentVertexStrides[nextAvailableIndex] = pointSpriteVertexStride;
        mCurrentVertexOffsets[nextAvailableIndex] = 0;

        if (!mPointSpriteIndexBuffer)
        {
            // Create an index buffer and set it for pointsprite rendering
            static const unsigned short pointSpriteIndices[] =
            {
                0, 1, 2, 3, 4, 5,
            };

            D3D11_SUBRESOURCE_DATA indexBufferData = { pointSpriteIndices, 0, 0 };
            D3D11_BUFFER_DESC indexBufferDesc;
            indexBufferDesc.ByteWidth = sizeof(pointSpriteIndices);
            indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
            indexBufferDesc.CPUAccessFlags = 0;
            indexBufferDesc.MiscFlags = 0;
            indexBufferDesc.StructureByteStride = 0;

            result = mDevice->CreateBuffer(&indexBufferDesc, &indexBufferData, &mPointSpriteIndexBuffer);
            if (FAILED(result))
            {
                SafeRelease(mPointSpriteVertexBuffer);
                return gl::Error(GL_OUT_OF_MEMORY, "Failed to create instanced pointsprite emulation index buffer, HRESULT: 0x%08x", result);
            }
        }

        // The index buffer is applied here because Instanced PointSprite emulation uses
        // the a non-indexed rendering path in ANGLE (DrawArrays).  This means that applyIndexBuffer()
        // on the renderer will not be called and setting this buffer here ensures that the rendering
        // path will contain the correct index buffers.
        mDeviceContext->IASetIndexBuffer(mPointSpriteIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    }

    if (moveFirstIndexedIntoSlotZero)
    {
        // In this case, we swapped the slots of the first instanced element and the first indexed element, to ensure
        // that the first slot contains non-instanced data (required by Feature Level 9_3).
        // We must also swap the corresponding buffers sent to IASetVertexBuffers so that the correct data is sent to each slot.
        std::swap(mCurrentBuffers[firstIndexedElement], mCurrentBuffers[firstInstancedElement]);
        std::swap(mCurrentVertexStrides[firstIndexedElement], mCurrentVertexStrides[firstInstancedElement]);
        std::swap(mCurrentVertexOffsets[firstIndexedElement], mCurrentVertexOffsets[firstInstancedElement]);
    }

    if (dirtyBuffers)
    {
        ASSERT(minDiff <= maxDiff && maxDiff < gl::MAX_VERTEX_ATTRIBS);
        mDeviceContext->IASetVertexBuffers(minDiff, maxDiff - minDiff + 1, mCurrentBuffers + minDiff,
                                           mCurrentVertexStrides + minDiff, mCurrentVertexOffsets + minDiff);
    }

    return gl::Error(GL_NO_ERROR);
}

}
