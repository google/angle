//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// InputLayoutCache.h: Defines InputLayoutCache, a class that builds and caches
// D3D11 input layouts.

#ifndef LIBANGLE_RENDERER_D3D_D3D11_INPUTLAYOUTCACHE_H_
#define LIBANGLE_RENDERER_D3D_D3D11_INPUTLAYOUTCACHE_H_

#include <GLES2/gl2.h>

#include <cstddef>

#include <array>
#include <map>

#include "common/angleutils.h"
#include "libANGLE/Constants.h"
#include "libANGLE/Error.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/d3d/RendererD3D.h"
#include "libANGLE/renderer/d3d/d3d11/ResourceManager11.h"

namespace gl
{
class Program;
}

namespace rx
{
struct TranslatedAttribute;
struct TranslatedIndexData;
struct SourceIndexData;
class ProgramD3D;
class Renderer11;

class InputLayoutCache : angle::NonCopyable
{
  public:
    InputLayoutCache();
    virtual ~InputLayoutCache();

    void initialize();
    void clear();
    void markDirty();

    gl::Error applyVertexBuffers(Renderer11 *renderer,
                                 const gl::State &state,
                                 const std::vector<TranslatedAttribute> &vertexArrayAttribs,
                                 const std::vector<TranslatedAttribute> &currentValueAttribs,
                                 GLenum mode,
                                 GLint start,
                                 TranslatedIndexData *indexInfo,
                                 GLsizei numIndicesPerInstance);

    gl::Error updateVertexOffsetsForPointSpritesEmulation(Renderer11 *renderer,
                                                          GLint startVertex,
                                                          GLsizei emulatedInstanceId);

    // Useful for testing
    void setCacheSize(unsigned int cacheSize) { mCacheSize = cacheSize; }

  private:
    struct PackedAttributeLayout
    {
        PackedAttributeLayout()
            : numAttributes(0),
              flags(0)
        {
        }

        void addAttributeData(GLenum glType,
                              UINT semanticIndex,
                              gl::VertexFormatType vertexFormatType,
                              unsigned int divisor);

        bool operator<(const PackedAttributeLayout &other) const;

        enum Flags
        {
            FLAG_USES_INSTANCED_SPRITES     = 0x1,
            FLAG_INSTANCED_SPRITES_ACTIVE   = 0x2,
            FLAG_INSTANCED_RENDERING_ACTIVE = 0x4,
        };

        size_t numAttributes;
        unsigned int flags;
        uint32_t attributeData[gl::MAX_VERTEX_ATTRIBS];
    };

    gl::Error updateInputLayout(Renderer11 *renderer,
                                const gl::State &state,
                                GLenum mode,
                                const AttribIndexArray &sortedSemanticIndices,
                                GLsizei numIndicesPerInstance);
    gl::Error createInputLayout(Renderer11 *renderer,
                                const AttribIndexArray &sortedSemanticIndices,
                                GLenum mode,
                                gl::Program *program,
                                GLsizei numIndicesPerInstance,
                                d3d11::InputLayout *inputLayoutOut);

    std::map<PackedAttributeLayout, d3d11::InputLayout> mLayoutMap;

    uintptr_t mCurrentIL;
    std::array<ID3D11Buffer *, gl::MAX_VERTEX_ATTRIBS> mCurrentBuffers;
    std::array<UINT, gl::MAX_VERTEX_ATTRIBS> mCurrentVertexStrides;
    std::array<UINT, gl::MAX_VERTEX_ATTRIBS> mCurrentVertexOffsets;
    std::vector<const TranslatedAttribute *> mCurrentAttributes;

    d3d11::Buffer mPointSpriteVertexBuffer;
    d3d11::Buffer mPointSpriteIndexBuffer;

    unsigned int mCacheSize;
};

}  // namespace rx

#endif // LIBANGLE_RENDERER_D3D_D3D11_INPUTLAYOUTCACHE_H_
