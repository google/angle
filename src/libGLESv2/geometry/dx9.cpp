//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// geometry/dx9.h: Direct3D 9-based implementation of BufferBackEnd, TranslatedVertexBuffer and TranslatedIndexBuffer.

#include "libGLESv2/geometry/dx9.h"

#include <cstddef>

#define GL_APICALL
#include <GLES2/gl2.h>

#include "common/debug.h"

#include "libGLESv2/Context.h"
#include "libGLESv2/geometry/vertexconversion.h"
#include "libGLESv2/geometry/IndexDataManager.h"

namespace
{
template <class InputType, template <std::size_t IncomingWidth> class WidenRule, class ElementConverter, class DefaultValueRule = gl::SimpleDefaultValues<InputType> >
class FormatConverterFactory
{
  private:
    template <std::size_t IncomingWidth>
    static gl::FormatConverter getFormatConverterForSize()
    {
        gl::FormatConverter formatConverter;

        formatConverter.identity = gl::VertexDataConverter<InputType, WidenRule<IncomingWidth>, ElementConverter, DefaultValueRule>::identity;
        formatConverter.outputVertexSize = gl::VertexDataConverter<InputType, WidenRule<IncomingWidth>, ElementConverter, DefaultValueRule>::finalSize;
        formatConverter.convertIndexed = gl::VertexDataConverter<InputType, WidenRule<IncomingWidth>, ElementConverter, DefaultValueRule>::convertIndexed;
        formatConverter.convertArray = gl::VertexDataConverter<InputType, WidenRule<IncomingWidth>, ElementConverter, DefaultValueRule>::convertArray;

        return formatConverter;
    }

  public:
    static gl::FormatConverter getFormatConverter(std::size_t size)
    {
        switch (size)
        {
          case 1: return getFormatConverterForSize<1>();
          case 2: return getFormatConverterForSize<2>();
          case 3: return getFormatConverterForSize<3>();
          case 4: return getFormatConverterForSize<4>();
          default: UNREACHABLE(); return getFormatConverterForSize<1>();
        }
    }
};
}

namespace gl
{
Dx9BackEnd::Dx9BackEnd(IDirect3DDevice9 *d3ddevice)
    : mDevice(d3ddevice)
{
    mDevice->AddRef();
}

Dx9BackEnd::~Dx9BackEnd()
{
    mDevice->Release();
}

TranslatedVertexBuffer *Dx9BackEnd::createVertexBuffer(std::size_t size)
{
    return new Dx9VertexBuffer(mDevice, size);
}

TranslatedVertexBuffer *Dx9BackEnd::createVertexBufferForStrideZero(std::size_t size)
{
    return new Dx9VertexBufferZeroStrideWorkaround(mDevice, size);
}

TranslatedIndexBuffer *Dx9BackEnd::createIndexBuffer(std::size_t size)
{
    return new Dx9IndexBuffer(mDevice, size);
}

// Mapping from OpenGL-ES vertex attrib type to D3D decl type:
//
// BYTE                 Translate to SHORT, expand to x2,x4 as needed.
// BYTE-norm            Translate to FLOAT since it can't be exactly represented as SHORT-norm.
// UNSIGNED_BYTE        x4 only. x1,x2,x3=>x4
// UNSIGNED_BYTE-norm   x4 only, x1,x2,x3=>x4
// SHORT                x2,x4 supported. x1=>x2, x3=>x4
// SHORT-norm           x2,x4 supported. x1=>x2, x3=>x4
// UNSIGNED_SHORT       unsupported, translate to float
// UNSIGNED_SHORT-norm  x2,x4 supported. x1=>x2, x3=>x4
// FIXED (not in WebGL) Translate to float.
// FLOAT                Fully supported.

FormatConverter Dx9BackEnd::getFormatConverter(GLenum type, std::size_t size, bool normalize)
{
    // FIXME: This should be rewritten to use C99 exact-sized types.
    switch (type)
    {
      case GL_BYTE:
        if (normalize)
        {
            return FormatConverterFactory<char, NoWiden, Normalize<char> >::getFormatConverter(size);
        }
        else
        {
            return FormatConverterFactory<char, WidenToEven, Cast<char, short> >::getFormatConverter(size);
        }

      case GL_UNSIGNED_BYTE:
        if (normalize)
        {
            return FormatConverterFactory<unsigned char, WidenToFour, Identity<unsigned char>, NormalizedDefaultValues<unsigned char> >::getFormatConverter(size);
        }
        else
        {
            return FormatConverterFactory<unsigned char, WidenToFour, Identity<unsigned char> >::getFormatConverter(size);
        }

      case GL_SHORT:
        if (normalize)
        {
            return FormatConverterFactory<short, WidenToEven, Identity<short>, NormalizedDefaultValues<short> >::getFormatConverter(size);
        }
        else
        {
            return FormatConverterFactory<short, WidenToEven, Identity<short> >::getFormatConverter(size);
        }

      case GL_UNSIGNED_SHORT:
        if (normalize)
        {
            return FormatConverterFactory<unsigned short, WidenToEven, Identity<unsigned short>, NormalizedDefaultValues<unsigned short> >::getFormatConverter(size);
        }
        else
        {
            return FormatConverterFactory<unsigned short, NoWiden, Cast<unsigned short, float> >::getFormatConverter(size);
        }

      case GL_FIXED:
        return FormatConverterFactory<int, NoWiden, FixedToFloat<int, 16> >::getFormatConverter(size);

      case GL_FLOAT:
        return FormatConverterFactory<float, NoWiden, Identity<float> >::getFormatConverter(size);

      default: UNREACHABLE(); return FormatConverterFactory<float, NoWiden, Identity<float> >::getFormatConverter(1);
    }
}

D3DDECLTYPE Dx9BackEnd::mapAttributeType(GLenum type, std::size_t size, bool normalized) const
{
    static const D3DDECLTYPE byteTypes[2][4] = { { D3DDECLTYPE_SHORT2, D3DDECLTYPE_SHORT2, D3DDECLTYPE_SHORT4, D3DDECLTYPE_SHORT4 }, { D3DDECLTYPE_FLOAT1, D3DDECLTYPE_FLOAT2, D3DDECLTYPE_FLOAT3, D3DDECLTYPE_FLOAT4 } };
    static const D3DDECLTYPE shortTypes[2][4] = { { D3DDECLTYPE_SHORT2, D3DDECLTYPE_SHORT2, D3DDECLTYPE_SHORT4, D3DDECLTYPE_SHORT4 }, { D3DDECLTYPE_SHORT2N, D3DDECLTYPE_SHORT2N, D3DDECLTYPE_SHORT4N, D3DDECLTYPE_SHORT4N } };
    static const D3DDECLTYPE ushortTypes[2][4] = { { D3DDECLTYPE_FLOAT1, D3DDECLTYPE_FLOAT2, D3DDECLTYPE_FLOAT3, D3DDECLTYPE_FLOAT4 }, { D3DDECLTYPE_USHORT2N, D3DDECLTYPE_USHORT2N, D3DDECLTYPE_USHORT4N, D3DDECLTYPE_USHORT4N } };

    static const D3DDECLTYPE floatTypes[4] =  { D3DDECLTYPE_FLOAT1, D3DDECLTYPE_FLOAT2, D3DDECLTYPE_FLOAT3, D3DDECLTYPE_FLOAT4 };

    switch (type)
    {
      case GL_BYTE: return byteTypes[normalized][size-1];
      case GL_UNSIGNED_BYTE: return normalized ? D3DDECLTYPE_UBYTE4N : D3DDECLTYPE_UBYTE4;
      case GL_SHORT: return shortTypes[normalized][size-1];
      case GL_UNSIGNED_SHORT: return ushortTypes[normalized][size-1];

      case GL_FIXED:
      case GL_FLOAT:
        return floatTypes[size-1];

      default:
        UNREACHABLE();
        return D3DDECLTYPE_FLOAT1;
    }
}

bool Dx9BackEnd::validateStream(GLenum type, std::size_t size, std::size_t stride, std::size_t offset) const
{
    // D3D9 requires the stream offset and stride to be a multiple of DWORD.
    return (stride % sizeof(DWORD) == 0 && offset % sizeof(DWORD) == 0);
}

IDirect3DVertexBuffer9 *Dx9BackEnd::getDxBuffer(TranslatedVertexBuffer *vb) const
{
    return vb ? static_cast<Dx9VertexBuffer*>(vb)->getBuffer() : NULL;
}

IDirect3DIndexBuffer9 *Dx9BackEnd::getDxBuffer(TranslatedIndexBuffer *ib) const
{
    return ib ? static_cast<Dx9IndexBuffer*>(ib)->getBuffer() : NULL;
}

GLenum Dx9BackEnd::setupIndicesPreDraw(const TranslatedIndexData &indexInfo)
{
    mDevice->SetIndices(getDxBuffer(indexInfo.buffer));
    return GL_NO_ERROR;
}

GLenum Dx9BackEnd::setupAttributesPreDraw(const TranslatedAttribute *attributes)
{
    HRESULT hr;

    D3DVERTEXELEMENT9 elements[MAX_VERTEX_ATTRIBS+1];

    D3DVERTEXELEMENT9 *nextElement = &elements[0];

    for (BYTE i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        if (attributes[i].enabled)
        {
            nextElement->Stream = i;
            nextElement->Offset = 0;
            nextElement->Type = static_cast<BYTE>(mapAttributeType(attributes[i].type, attributes[i].size, attributes[i].normalized));
            nextElement->Method = D3DDECLMETHOD_DEFAULT;
            nextElement->Usage = D3DDECLUSAGE_TEXCOORD;
            nextElement->UsageIndex = attributes[i].programAttribute;
            nextElement++;
        }
    }

    static const D3DVERTEXELEMENT9 end = D3DDECL_END();
    *nextElement = end;

    IDirect3DVertexDeclaration9* vertexDeclaration;
    hr = mDevice->CreateVertexDeclaration(elements, &vertexDeclaration);
    mDevice->SetVertexDeclaration(vertexDeclaration);
    vertexDeclaration->Release();

    for (size_t i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        if (attributes[i].enabled)
        {
            mDevice->SetStreamSource(i, getDxBuffer(attributes[i].buffer), attributes[i].offset, attributes[i].stride);
        }
        else
        {
            mDevice->SetStreamSource(i, 0, 0, 0);
        }
    }

    return GL_NO_ERROR;
}

Dx9BackEnd::Dx9VertexBuffer::Dx9VertexBuffer(IDirect3DDevice9 *device, std::size_t size)
    : TranslatedVertexBuffer(size)
{
    HRESULT hr = device->CreateVertexBuffer(size, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &mVertexBuffer, NULL);
    if (hr != S_OK)
    {
        ERR("Out of memory allocating a vertex buffer of size %lu.", size);
        throw std::bad_alloc();
    }
}

Dx9BackEnd::Dx9VertexBuffer::Dx9VertexBuffer(IDirect3DDevice9 *device, std::size_t size, DWORD usageFlags)
    : TranslatedVertexBuffer(size)
{
    HRESULT hr = device->CreateVertexBuffer(size, usageFlags, 0, D3DPOOL_DEFAULT, &mVertexBuffer, NULL);
    if (hr != S_OK)
    {
        ERR("Out of memory allocating a vertex buffer of size %lu.", size);
        throw std::bad_alloc();
    }
}


Dx9BackEnd::Dx9VertexBuffer::~Dx9VertexBuffer()
{
    mVertexBuffer->Release();
}

IDirect3DVertexBuffer9 *Dx9BackEnd::Dx9VertexBuffer::getBuffer() const
{
    return mVertexBuffer;
}

void *Dx9BackEnd::Dx9VertexBuffer::map()
{
    void *mapPtr;

    mVertexBuffer->Lock(0, 0, &mapPtr, 0);

    return mapPtr;
}

void Dx9BackEnd::Dx9VertexBuffer::unmap()
{
    mVertexBuffer->Unlock();
}

void Dx9BackEnd::Dx9VertexBuffer::recycle()
{
    void *dummy;
    mVertexBuffer->Lock(0, 1, &dummy, D3DLOCK_DISCARD);
    mVertexBuffer->Unlock();
}

void *Dx9BackEnd::Dx9VertexBuffer::streamingMap(std::size_t offset, std::size_t size)
{
    void *mapPtr;

    mVertexBuffer->Lock(offset, size, &mapPtr, D3DLOCK_NOOVERWRITE);

    return mapPtr;
}

// Normally VBs are created with D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, but some hardware & drivers won't render
// if any stride-zero streams are in D3DUSAGE_DYNAMIC VBs, so this provides a way to create such VBs with only D3DUSAGE_WRITEONLY set.
// D3DLOCK_DISCARD and D3DLOCK_NOOVERWRITE are only available on D3DUSAGE_DYNAMIC VBs, so we override methods to avoid using these flags.
Dx9BackEnd::Dx9VertexBufferZeroStrideWorkaround::Dx9VertexBufferZeroStrideWorkaround(IDirect3DDevice9 *device, std::size_t size)
    : Dx9VertexBuffer(device, size, D3DUSAGE_WRITEONLY)
{
}

void Dx9BackEnd::Dx9VertexBufferZeroStrideWorkaround::recycle()
{
}

void *Dx9BackEnd::Dx9VertexBufferZeroStrideWorkaround::streamingMap(std::size_t offset, std::size_t size)
{
    void *mapPtr;

    getBuffer()->Lock(offset, size, &mapPtr, 0);

    return mapPtr;
}

Dx9BackEnd::Dx9IndexBuffer::Dx9IndexBuffer(IDirect3DDevice9 *device, std::size_t size)
    : TranslatedIndexBuffer(size)
{
    HRESULT hr = device->CreateIndexBuffer(size, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &mIndexBuffer, NULL);
    if (hr != S_OK)
    {
        ERR("Out of memory allocating an index buffer of size %lu.", size);
        throw std::bad_alloc();
    }
}

Dx9BackEnd::Dx9IndexBuffer::~Dx9IndexBuffer()
{
    mIndexBuffer->Release();
}

IDirect3DIndexBuffer9*Dx9BackEnd::Dx9IndexBuffer::getBuffer() const
{
    return mIndexBuffer;
}

void *Dx9BackEnd::Dx9IndexBuffer::map()
{
    void *mapPtr;

    mIndexBuffer->Lock(0, 0, &mapPtr, 0);

    return mapPtr;
}

void Dx9BackEnd::Dx9IndexBuffer::unmap()
{
    mIndexBuffer->Unlock();
}

void Dx9BackEnd::Dx9IndexBuffer::recycle()
{
    void *dummy;
    mIndexBuffer->Lock(0, 1, &dummy, D3DLOCK_DISCARD);
    mIndexBuffer->Unlock();
}

void *Dx9BackEnd::Dx9IndexBuffer::streamingMap(std::size_t offset, std::size_t size)
{
    void *mapPtr;

    mIndexBuffer->Lock(offset, size, &mapPtr, D3DLOCK_NOOVERWRITE);

    return mapPtr;
}

}
