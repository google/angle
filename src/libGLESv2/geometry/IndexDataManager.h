//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// geometry/IndexDataManager.h: Defines the IndexDataManager, a class that
// runs the Buffer translation process for index buffers.

#ifndef LIBGLESV2_GEOMETRY_INDEXDATAMANAGER_H_
#define LIBGLESV2_GEOMETRY_INDEXDATAMANAGER_H_

#include <bitset>
#include <cstddef>

#define GL_APICALL
#include <GLES2/gl2.h>

#include "Context.h"

namespace gl
{

class Buffer;
class BufferBackEnd;
class TranslatedIndexBuffer;
struct FormatConverter;

struct TranslatedIndexData
{
    GLuint minIndex;
    GLuint maxIndex;
    GLuint count;

    const Index *indices;

    TranslatedIndexBuffer *buffer;
    GLsizei offset;
};

class IndexDataManager
{
  public:
    IndexDataManager(Context *context, BufferBackEnd *backend);
    ~IndexDataManager();

    TranslatedIndexData preRenderValidate(GLenum mode, GLenum type, GLsizei count, Buffer *arrayElementBuffer, const void *indices);

  private:
    std::size_t spaceRequired(GLenum mode, GLenum type, GLsizei count);

    Context *mContext;
    BufferBackEnd *mBackend;

    TranslatedIndexBuffer *mStreamBuffer;
};

}

#endif   // LIBGLESV2_GEOMETRY_INDEXDATAMANAGER_H_
