//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexDataManager.h: Defines the VertexDataManager, a class that
// runs the Buffer translation process.

#ifndef LIBGLESV2_RENDERER_VERTEXDATAMANAGER_H_
#define LIBGLESV2_RENDERER_VERTEXDATAMANAGER_H_

#include <vector>
#include <cstddef>

#define GL_APICALL
#include <GLES2/gl2.h>

#include "libGLESv2/Context.h"
#include "libGLESv2/renderer/VertexBuffer.h"

namespace rx
{

struct TranslatedAttribute
{
    bool active;

    D3DDECLTYPE type;
    UINT offset;
    UINT stride;   // 0 means not to advance the read pointer at all

    IDirect3DVertexBuffer9 *vertexBuffer;
    unsigned int serial;
    unsigned int divisor;
};

class VertexDataManager
{
  public:
    VertexDataManager(rx::Renderer9 *renderer);
    virtual ~VertexDataManager();

    GLenum prepareVertexData(const gl::VertexAttribute attribs[], gl::ProgramBinary *programBinary, GLint start, GLsizei count, TranslatedAttribute *outAttribs, GLsizei instances);

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexDataManager);

    std::size_t spaceRequired(const gl::VertexAttribute &attrib, std::size_t count, GLsizei instances) const;
    std::size_t writeAttributeData(VertexBuffer *vertexBuffer, GLint start, GLsizei count, const gl::VertexAttribute &attribute, GLsizei instances);

    rx::Renderer9 *const mRenderer;   // D3D9_REPLACE

    StreamingVertexBuffer *mStreamingBuffer;

    float mCurrentValue[gl::MAX_VERTEX_ATTRIBS][4];
    StreamingVertexBuffer *mCurrentValueBuffer[gl::MAX_VERTEX_ATTRIBS];
    std::size_t mCurrentValueOffsets[gl::MAX_VERTEX_ATTRIBS];

    // Attribute format conversion
    struct FormatConverter
    {
        bool identity;
        std::size_t outputElementSize;
        void (*convertArray)(const void *in, std::size_t stride, std::size_t n, void *out);
        D3DDECLTYPE d3dDeclType;
    };

    enum { NUM_GL_VERTEX_ATTRIB_TYPES = 6 };

    FormatConverter mAttributeTypes[NUM_GL_VERTEX_ATTRIB_TYPES][2][4];   // [GL types as enumerated by typeIndex()][normalized][size - 1]

    struct TranslationDescription
    {
        DWORD capsFlag;
        FormatConverter preferredConversion;
        FormatConverter fallbackConversion;
    };

    // This table is used to generate mAttributeTypes.
    static const TranslationDescription mPossibleTranslations[NUM_GL_VERTEX_ATTRIB_TYPES][2][4]; // [GL types as enumerated by typeIndex()][normalized][size - 1]

    void checkVertexCaps(DWORD declTypes);

    unsigned int typeIndex(GLenum type) const;
    const FormatConverter &formatConverter(const gl::VertexAttribute &attribute) const;
};

}

#endif   // LIBGLESV2_RENDERER_VERTEXDATAMANAGER_H_
