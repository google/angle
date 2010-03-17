//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// geometry/dx9.h: Direct3D 9-based implementation of BufferBackEnd, TranslatedVertexBuffer and TranslatedIndexBuffer.

#ifndef LIBGLESV2_GEOMETRY_DX9_H_
#define LIBGLESV2_GEOMETRY_DX9_H_

#include <d3d9.h>

#include "Buffer.h"
#include "geometry/backend.h"

namespace gl
{

class Dx9BackEnd : public BufferBackEnd
{
  public:
    explicit Dx9BackEnd(IDirect3DDevice9 *d3ddevice);
    ~Dx9BackEnd();

    virtual TranslatedVertexBuffer *createVertexBuffer(std::size_t size);
    virtual TranslatedIndexBuffer *createIndexBuffer(std::size_t size);
    virtual FormatConverter getFormatConverter(GLenum type, std::size_t size, bool normalize);

    virtual GLenum preDraw(const TranslatedAttribute *attributes);

  private:
    IDirect3DDevice9 *mDevice;

    class Dx9VertexBuffer : public TranslatedVertexBuffer
    {
      public:
        Dx9VertexBuffer(IDirect3DDevice9 *device, std::size_t size);
        virtual ~Dx9VertexBuffer();

        IDirect3DVertexBuffer9 *getBuffer() const;

      protected:
        virtual void *map();
        virtual void unmap();

        virtual void recycle();
        virtual void *streamingMap(std::size_t offset, std::size_t size);

      private:
        IDirect3DVertexBuffer9 *mVertexBuffer;
    };

    class Dx9IndexBuffer : public TranslatedIndexBuffer
    {
      public:
        Dx9IndexBuffer(IDirect3DDevice9 *device, std::size_t size);
        virtual ~Dx9IndexBuffer();

        IDirect3DIndexBuffer9 *getBuffer() const;

      protected:
        virtual void *map();
        virtual void unmap();

        virtual void recycle();
        virtual void *streamingMap(std::size_t offset, std::size_t size);

      private:
        IDirect3DIndexBuffer9 *mIndexBuffer;
    };

    IDirect3DVertexBuffer9 *getDxBuffer(TranslatedVertexBuffer *vb) const;
    IDirect3DIndexBuffer9 *getDxBuffer(TranslatedIndexBuffer *ib) const;

    D3DDECLTYPE mapAttributeType(GLenum type, std::size_t size, bool normalized) const;
};

}

#endif   // LIBGLESV2_GEOMETRY_DX9_H_
