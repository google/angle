//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// geometry/VertexDataManager.h: Defines the VertexDataManager, a class that
// runs the Buffer translation process.

#ifndef LIBGLESV2_GEOMETRY_VERTEXDATAMANAGER_H_
#define LIBGLESV2_GEOMETRY_VERTEXDATAMANAGER_H_

#include <bitset>
#include <cstddef>

#define GL_APICALL
#include <GLES2/gl2.h>

#include "Context.h"

namespace gl
{

class Buffer;
class BufferBackEnd;
class TranslatedVertexBuffer;
struct TranslatedAttribute;
struct FormatConverter;

class VertexDataManager
{
  public:
    VertexDataManager(Context *context, BufferBackEnd *backend);
    ~VertexDataManager();

    void dirtyCurrentValues() { mDirtyCurrentValues = true; }

    GLenum preRenderValidate(GLint start,
                             GLsizei count,
                             TranslatedAttribute *outAttribs);

    GLenum preRenderValidate(const Index *indices,
                             GLsizei count,
                             TranslatedAttribute* outAttribs);

  private:
    std::bitset<MAX_VERTEX_ATTRIBS> activeAttribs();

    class TranslationHelper
    {
      public:
        virtual ~TranslationHelper() { }

        virtual void translate(const FormatConverter &converter, GLsizei stride, const void *source, void *dest) = 0;
    };

    class ArrayTranslationHelper : public TranslationHelper
    {
      public:
        ArrayTranslationHelper(GLint first, GLsizei count);

        void translate(const FormatConverter &converter, GLsizei stride, const void *source, void *dest);

      private:
        GLint mFirst;
        GLsizei mCount;
    };

    class IndexedTranslationHelper : public TranslationHelper
    {
      public:
        IndexedTranslationHelper(const Index *indices, GLsizei count);

        void translate(const FormatConverter &converter, GLint stride, const void *source, void *dest);

      private:
        const Index *mIndices;
        GLsizei mCount;
    };

    GLenum internalPreRenderValidate(const AttributeState *attribs,
                                     const std::bitset<MAX_VERTEX_ATTRIBS> &activeAttribs,
                                     Index minIndex,
                                     Index maxIndex,
                                     TranslationHelper *translator,
                                     TranslatedAttribute *outAttribs);

    void reloadCurrentValues(const AttributeState *attribs, std::size_t *offset);

    void processNonArrayAttributes(const AttributeState *attribs, const std::bitset<MAX_VERTEX_ATTRIBS> &activeAttribs, TranslatedAttribute *translated);

    std::size_t typeSize(GLenum type) const;
    std::size_t interpretGlStride(const AttributeState &attrib) const;

    std::size_t roundUp(std::size_t x, std::size_t multiple) const;
    std::size_t spaceRequired(const AttributeState &attrib, std::size_t maxVertex) const;

    Context *mContext;
    BufferBackEnd *mBackend;

    TranslatedVertexBuffer *mCurrentValueBuffer;

    TranslatedVertexBuffer *mStreamBuffer;

    bool mDirtyCurrentValues;
};

}

#endif   // LIBGLESV2_GEOMETRY_VERTEXDATAMANAGER_H_
