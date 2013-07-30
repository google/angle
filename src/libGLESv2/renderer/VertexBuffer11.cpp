#include "precompiled.h"
//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexBuffer11.cpp: Defines the D3D11 VertexBuffer implementation.

#include "libGLESv2/renderer/VertexBuffer11.h"
#include "libGLESv2/renderer/BufferStorage.h"

#include "libGLESv2/Buffer.h"
#include "libGLESv2/renderer/Renderer11.h"
#include "libGLESv2/VertexAttribute.h"

namespace
{

unsigned int GetIntegerTypeIndex(GLenum type)
{
    switch (type)
    {
      case GL_BYTE:                        return 0;
      case GL_UNSIGNED_BYTE:               return 1;
      case GL_SHORT:                       return 2;
      case GL_UNSIGNED_SHORT:              return 3;
      case GL_INT:                         return 4;
      case GL_UNSIGNED_INT:                return 5;
      case GL_INT_2_10_10_10_REV:          return 6;
      case GL_UNSIGNED_INT_2_10_10_10_REV: return 7;
      default:                             UNREACHABLE(); return 0;
    }
}

unsigned int GetFloatTypeIndex(GLenum type)
{
    switch (type)
    {
      case GL_BYTE:                        return 0;
      case GL_UNSIGNED_BYTE:               return 1;
      case GL_SHORT:                       return 2;
      case GL_UNSIGNED_SHORT:              return 3;
      case GL_INT:                         return 4;
      case GL_UNSIGNED_INT:                return 5;
      case GL_INT_2_10_10_10_REV:          return 6;
      case GL_UNSIGNED_INT_2_10_10_10_REV: return 7;
      case GL_FIXED:                       return 8;
      case GL_HALF_FLOAT:                  return 9;
      case GL_FLOAT:                       return 10;
      default:                             UNREACHABLE(); return 0;
    }
}

}

namespace rx
{

VertexBuffer11::VertexBuffer11(rx::Renderer11 *const renderer) : mRenderer(renderer)
{
    mBuffer = NULL;
    mBufferSize = 0;
    mDynamicUsage = false;
}

VertexBuffer11::~VertexBuffer11()
{
    SafeRelease(mBuffer);
}

bool VertexBuffer11::initialize(unsigned int size, bool dynamicUsage)
{
    SafeRelease(mBuffer);

    updateSerial();

    if (size > 0)
    {
        ID3D11Device* dxDevice = mRenderer->getDevice();

        D3D11_BUFFER_DESC bufferDesc;
        bufferDesc.ByteWidth = size;
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        bufferDesc.MiscFlags = 0;
        bufferDesc.StructureByteStride = 0;

        HRESULT result = dxDevice->CreateBuffer(&bufferDesc, NULL, &mBuffer);
        if (FAILED(result))
        {
            return false;
        }
    }

    mBufferSize = size;
    mDynamicUsage = dynamicUsage;
    return true;
}

VertexBuffer11 *VertexBuffer11::makeVertexBuffer11(VertexBuffer *vetexBuffer)
{
    ASSERT(HAS_DYNAMIC_TYPE(VertexBuffer11*, vetexBuffer));
    return static_cast<VertexBuffer11*>(vetexBuffer);
}

bool VertexBuffer11::storeVertexAttributes(const gl::VertexAttribute &attrib, const gl::VertexAttribCurrentValueData &currentValue,
                                           GLint start, GLsizei count, GLsizei instances, unsigned int offset)
{
    if (mBuffer)
    {
        gl::Buffer *buffer = attrib.mBoundBuffer.get();

        int inputStride = attrib.stride();
        const VertexConverter &converter = attrib.mArrayEnabled ?
                                           getVertexConversion(attrib) :
                                           getVertexConversion(currentValue.Type, currentValue.Type != GL_FLOAT, false, 4);

        ID3D11DeviceContext *dxContext = mRenderer->getDeviceContext();

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT result = dxContext->Map(mBuffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedResource);
        if (FAILED(result))
        {
            ERR("Vertex buffer map failed with error 0x%08x", result);
            return false;
        }

        char* output = reinterpret_cast<char*>(mappedResource.pData) + offset;

        const char *input = NULL;
        if (attrib.mArrayEnabled)
        {
            if (buffer)
            {
                BufferStorage *storage = buffer->getStorage();
                input = static_cast<const char*>(storage->getData()) + static_cast<int>(attrib.mOffset);
            }
            else
            {
                input = static_cast<const char*>(attrib.mPointer);
            }
        }
        else
        {
            input = reinterpret_cast<const char*>(currentValue.FloatValues);
        }

        if (instances == 0 || attrib.mDivisor == 0)
        {
            input += inputStride * start;
        }

        ASSERT(converter.conversionFunc != NULL);
        converter.conversionFunc(input, inputStride, count, output);

        dxContext->Unmap(mBuffer, 0);

        return true;
    }
    else
    {
        ERR("Vertex buffer not initialized.");
        return false;
    }
}

unsigned int VertexBuffer11::getSpaceRequired(const gl::VertexAttribute &attrib, GLsizei count,
                                              GLsizei instances) const
{
    if (attrib.mArrayEnabled)
    {
        unsigned int elementSize = getVertexConversion(attrib).outputElementSize;

        if (instances == 0 || attrib.mDivisor == 0)
        {
            return elementSize * count;
        }
        else
        {
            return elementSize * ((instances + attrib.mDivisor - 1) / attrib.mDivisor);
        }
    }
    else
    {
        unsigned int elementSize = 4;
        return elementSize * 4;
    }
}

bool VertexBuffer11::requiresConversion(const gl::VertexAttribute &attrib) const
{
    return !getVertexConversion(attrib).identity;
}

bool VertexBuffer11::requiresConversion(const gl::VertexAttribCurrentValueData &currentValue) const
{
    return !getVertexConversion(currentValue.Type, currentValue.Type != GL_FLOAT, false, 4).identity;
}

unsigned int VertexBuffer11::getBufferSize() const
{
    return mBufferSize;
}

bool VertexBuffer11::setBufferSize(unsigned int size)
{
    if (size > mBufferSize)
    {
        return initialize(size, mDynamicUsage);
    }
    else
    {
        return true;
    }
}

bool VertexBuffer11::discard()
{
    if (mBuffer)
    {
        ID3D11DeviceContext *dxContext = mRenderer->getDeviceContext();

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT result = dxContext->Map(mBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (FAILED(result))
        {
            ERR("Vertex buffer map failed with error 0x%08x", result);
            return false;
        }

        dxContext->Unmap(mBuffer, 0);

        return true;
    }
    else
    {
        ERR("Vertex buffer not initialized.");
        return false;
    }
}

DXGI_FORMAT VertexBuffer11::getAttributeDXGIFormat(const gl::VertexAttribute &attrib)
{
    return getVertexConversion(attrib).dxgiFormat;
}

DXGI_FORMAT VertexBuffer11::getCurrentValueDXGIFormat(GLenum currentValueType)
{
    if (currentValueType == GL_FLOAT)
    {
        return mFloatVertexTranslations[GetFloatTypeIndex(GL_FLOAT)][0][3].dxgiFormat;
    }
    else
    {
        ASSERT(currentValueType == GL_INT || currentValueType == GL_UNSIGNED_INT);
        return mIntegerVertexTranslations[GetIntegerTypeIndex(currentValueType)][3].dxgiFormat;
    }
}

ID3D11Buffer *VertexBuffer11::getBuffer() const
{
    return mBuffer;
}

template <typename T, unsigned int componentCount, bool widen, unsigned int defaultValueBits>
static void copyVertexData(const void *input, unsigned int stride, unsigned int count, void *output)
{
    const unsigned int attribSize = sizeof(T) * componentCount;

    const unsigned int defaultBits = defaultValueBits;
    const T defaultValue = *reinterpret_cast<const T*>(&defaultBits);

    if (attribSize == stride && !widen)
    {
        memcpy(output, input, count * attribSize);
    }
    else
    {
        unsigned int outputStride = widen ? 4 : componentCount;

        for (unsigned int i = 0; i < count; i++)
        {
            const T *offsetInput = reinterpret_cast<const T*>(reinterpret_cast<const char*>(input) + i * stride);
            T *offsetOutput = reinterpret_cast<T*>(output) + i * outputStride;

            for (unsigned int j = 0; j < componentCount; j++)
            {
                offsetOutput[j] = offsetInput[j];
            }

            if (widen)
            {
                offsetOutput[3] = defaultValue;
            }
        }
    }
}

template <unsigned int componentCount>
static void copyFixedVertexData(const void* input, unsigned int stride, unsigned int count, void* output)
{
    static const float divisor = 1.0f / (1 << 16);

    for (unsigned int i = 0; i < count; i++)
    {
        const GLfixed* offsetInput = reinterpret_cast<const GLfixed*>(reinterpret_cast<const char*>(input) + stride * i);
        float* offsetOutput = reinterpret_cast<float*>(output) + i * componentCount;

        for (unsigned int j = 0; j < componentCount; j++)
        {
            offsetOutput[j] = static_cast<float>(offsetInput[j]) * divisor;
        }
    }
}

template <typename T, unsigned int componentCount, bool normalized>
static void copyToFloatVertexData(const void* input, unsigned int stride, unsigned int count, void* output)
{
    typedef std::numeric_limits<T> NL;

    for (unsigned int i = 0; i < count; i++)
    {
        const T *offsetInput = reinterpret_cast<const T*>(reinterpret_cast<const char*>(input) + stride * i);
        float *offsetOutput = reinterpret_cast<float*>(output) + i * componentCount;

        for (unsigned int j = 0; j < componentCount; j++)
        {
            if (normalized)
            {
                if (NL::is_signed)
                {
                    const float divisor = 1.0f / (2 * static_cast<float>(NL::max()) + 1);
                    offsetOutput[j] = (2 * static_cast<float>(offsetInput[j]) + 1) * divisor;
                }
                else
                {
                    offsetOutput[j] =  static_cast<float>(offsetInput[j]) / NL::max();
                }
            }
            else
            {
                offsetOutput[j] =  static_cast<float>(offsetInput[j]);
            }
        }
    }
}

static void copyPackedUnsignedVertexData(const void* input, unsigned int stride, unsigned int count, void* output)
{
    const unsigned int attribSize = 4;

    if (attribSize == stride)
    {
        memcpy(output, input, count * attribSize);
    }
    else
    {
        for (unsigned int i = 0; i < count; i++)
        {
            const GLuint *offsetInput = reinterpret_cast<const GLuint*>(reinterpret_cast<const char*>(input) + (i * stride));
            GLuint *offsetOutput = reinterpret_cast<GLuint*>(output) + (i * attribSize);

            offsetOutput[i] = offsetInput[i];
        }
    }
}

template <bool isSigned, bool normalized, bool toFloat>
static inline void copyPackedRGB(unsigned int data, void *output)
{
    const unsigned int rgbSignMask = 0x200;       // 1 set at the 9 bit
    const unsigned int negativeMask = 0xFFFFFC00; // All bits from 10 to 31 set to 1

    if (toFloat)
    {
        GLfloat *floatOutput = reinterpret_cast<GLfloat*>(output);
        if (isSigned)
        {
            GLfloat finalValue = 0;
            if (data & rgbSignMask)
            {
                int negativeNumber = data | negativeMask;
                finalValue = static_cast<GLfloat>(negativeNumber);
            }
            else
            {
                finalValue = static_cast<GLfloat>(data);
            }

            if (normalized)
            {
                const int maxValue = 0x1FF;      // 1 set in bits 0 through 8
                const int minValue = 0xFFFFFE01; // Inverse of maxValue

                // A 10-bit two's complement number has the possibility of being minValue - 1 but
                // OpenGL's normalization rules dictate that it should be clamped to minValue in this
                // case.
                if (finalValue < minValue)
                {
                    finalValue = minValue;
                }

                const int halfRange = (maxValue - minValue) >> 1;
                *floatOutput = ((finalValue - minValue) / halfRange) - 1.0f;
            }
            else
            {
                *floatOutput = finalValue;
            }
        }
        else
        {
            if (normalized)
            {
                const unsigned int maxValue = 0x3FF; // 1 set in bits 0 through 9
                *floatOutput = static_cast<GLfloat>(data) / static_cast<GLfloat>(maxValue);
            }
            else
            {
                *floatOutput = static_cast<GLfloat>(data);
            }
        }
    }
    else
    {
        if (isSigned)
        {
            GLshort *intOutput = reinterpret_cast<GLshort*>(output);

            if (data & rgbSignMask)
            {
                *intOutput = data | negativeMask;
            }
            else
            {
                *intOutput = data;
            }
        }
        else
        {
            GLushort *uintOutput = reinterpret_cast<GLushort*>(output);
            *uintOutput = data;
        }
    }
}

template <bool isSigned, bool normalized, bool toFloat>
static inline void copyPackedAlpha(unsigned int data, void *output)
{
    if (toFloat)
    {
        GLfloat *floatOutput = reinterpret_cast<GLfloat*>(output);
        if (isSigned)
        {
            if (normalized)
            {
                switch (data)
                {
                  case 0x0: *floatOutput =  0.0f; break;
                  case 0x1: *floatOutput =  1.0f; break;
                  case 0x2: *floatOutput = -1.0f; break;
                  case 0x3: *floatOutput = -1.0f; break;
                  default: UNREACHABLE();
                }
            }
            else
            {
                switch (data)
                {
                  case 0x0: *floatOutput =  0.0f; break;
                  case 0x1: *floatOutput =  1.0f; break;
                  case 0x2: *floatOutput = -2.0f; break;
                  case 0x3: *floatOutput = -1.0f; break;
                  default: UNREACHABLE();
                }
            }
        }
        else
        {
            if (normalized)
            {
                switch (data)
                {
                  case 0x0: *floatOutput = 0.0f / 3.0f; break;
                  case 0x1: *floatOutput = 1.0f / 3.0f; break;
                  case 0x2: *floatOutput = 2.0f / 3.0f; break;
                  case 0x3: *floatOutput = 3.0f / 3.0f; break;
                  default: UNREACHABLE();
                }
            }
            else
            {
                switch (data)
                {
                  case 0x0: *floatOutput = 0.0f; break;
                  case 0x1: *floatOutput = 1.0f; break;
                  case 0x2: *floatOutput = 2.0f; break;
                  case 0x3: *floatOutput = 3.0f; break;
                  default: UNREACHABLE();
                }
            }
        }
    }
    else
    {
        if (isSigned)
        {
            GLshort *intOutput = reinterpret_cast<GLshort*>(output);
            switch (data)
            {
              case 0x0: *intOutput =  0; break;
              case 0x1: *intOutput =  1; break;
              case 0x2: *intOutput = -2; break;
              case 0x3: *intOutput = -1; break;
              default: UNREACHABLE();
            }
        }
        else
        {
            GLushort *uintOutput = reinterpret_cast<GLushort*>(output);
            switch (data)
            {
              case 0x0: *uintOutput = 0; break;
              case 0x1: *uintOutput = 1; break;
              case 0x2: *uintOutput = 2; break;
              case 0x3: *uintOutput = 3; break;
              default: UNREACHABLE();
            }
        }
    }
}

template <bool isSigned, bool normalized, bool toFloat>
static void copyPackedVertexData(const void* input, unsigned int stride, unsigned int count, void* output)
{
    const unsigned int outputComponentSize = toFloat ? 4 : 2;
    const unsigned int componentCount = 4;

    const unsigned int rgbMask = 0x3FF; // 1 set in bits 0 through 9
    const unsigned int redShift = 0;    // red is bits 0 through 9
    const unsigned int greenShift = 10; // green is bits 10 through 19
    const unsigned int blueShift = 20;  // blue is bits 20 through 29

    const unsigned int alphaMask = 0x3; // 1 set in bits 0 and 1
    const unsigned int alphaShift = 30; // Alpha is the 30 and 31 bits

    for (unsigned int i = 0; i < count; i++)
    {
        GLuint packedValue = *reinterpret_cast<const GLuint*>(reinterpret_cast<const char*>(input) + (i * stride));
        GLbyte *offsetOutput = reinterpret_cast<GLbyte*>(output) + (i * outputComponentSize * componentCount);

        copyPackedRGB<isSigned, normalized, toFloat>(  (packedValue >> redShift) & rgbMask,     offsetOutput + (0 * outputComponentSize));
        copyPackedRGB<isSigned, normalized, toFloat>(  (packedValue >> greenShift) & rgbMask,   offsetOutput + (1 * outputComponentSize));
        copyPackedRGB<isSigned, normalized, toFloat>(  (packedValue >> blueShift) & rgbMask,    offsetOutput + (2 * outputComponentSize));
        copyPackedAlpha<isSigned, normalized, toFloat>((packedValue >> alphaShift) & alphaMask, offsetOutput + (3* outputComponentSize));
    }
}

const VertexBuffer11::VertexConverter VertexBuffer11::mFloatVertexTranslations[NUM_GL_FLOAT_VERTEX_ATTRIB_TYPES][2][4] =
{
    { // GL_BYTE
        { // unnormalized
            { &copyToFloatVertexData<GLbyte, 1, false>, false, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyToFloatVertexData<GLbyte, 2, false>, false, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyToFloatVertexData<GLbyte, 3, false>, false, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyToFloatVertexData<GLbyte, 4, false>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
        { // normalized
            { &copyVertexData<GLbyte, 1, false, INT8_MAX>, true, DXGI_FORMAT_R8_SNORM, 1 },
            { &copyVertexData<GLbyte, 2, false, INT8_MAX>, true, DXGI_FORMAT_R8G8_SNORM, 2 },
            { &copyVertexData<GLbyte, 3, true, INT8_MAX>, false, DXGI_FORMAT_R8G8B8A8_SNORM, 4 },
            { &copyVertexData<GLbyte, 4, false, INT8_MAX>, true, DXGI_FORMAT_R8G8B8A8_SNORM, 4 },
        },
    },
    { // GL_UNSIGNED_BYTE
        { // unnormalized
            { &copyToFloatVertexData<GLubyte, 1, false>, false, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyToFloatVertexData<GLubyte, 2, false>, false, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyToFloatVertexData<GLubyte, 3, false>, false, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyToFloatVertexData<GLubyte, 4, false>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
        { // normalized
            { &copyVertexData<GLubyte, 1, false, UINT8_MAX>, true, DXGI_FORMAT_R8_UNORM, 1 },
            { &copyVertexData<GLubyte, 2, false, UINT8_MAX>, true, DXGI_FORMAT_R8G8_UNORM, 2 },
            { &copyVertexData<GLubyte, 3, true, UINT8_MAX>, false, DXGI_FORMAT_R8G8B8A8_UNORM, 4 },
            { &copyVertexData<GLubyte, 4, false, UINT8_MAX>, true, DXGI_FORMAT_R8G8B8A8_UNORM, 4 },
        },
    },
    { // GL_SHORT
        { // unnormalized
            { &copyToFloatVertexData<GLshort, 1, false>, false, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyToFloatVertexData<GLshort, 2, false>, false, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyToFloatVertexData<GLshort, 3, false>, false, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyToFloatVertexData<GLshort, 4, false>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
        { // normalized
            { &copyVertexData<GLshort, 1, false, INT16_MAX>, true, DXGI_FORMAT_R16_SNORM, 2 },
            { &copyVertexData<GLshort, 2, false, INT16_MAX>, true, DXGI_FORMAT_R16G16_SNORM, 4 },
            { &copyVertexData<GLshort, 3, true, INT16_MAX>, false, DXGI_FORMAT_R16G16B16A16_SNORM, 8 },
            { &copyVertexData<GLshort, 4, false, INT16_MAX>, true, DXGI_FORMAT_R16G16B16A16_SNORM, 8 },
        },
    },
    { // GL_UNSIGNED_SHORT
        { // unnormalized
            { &copyToFloatVertexData<GLushort, 1, false>, false, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyToFloatVertexData<GLushort, 2, false>, false, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyToFloatVertexData<GLushort, 3, false>, false, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyToFloatVertexData<GLushort, 4, false>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
        { // normalized
            { &copyVertexData<GLushort, 1, false, UINT16_MAX>, true, DXGI_FORMAT_R16_UNORM, 2 },
            { &copyVertexData<GLushort, 2, false, UINT16_MAX>, true, DXGI_FORMAT_R16G16_UNORM, 4 },
            { &copyVertexData<GLushort, 3, true, UINT16_MAX>, false, DXGI_FORMAT_R16G16B16A16_UNORM, 8 },
            { &copyVertexData<GLushort, 4, false, UINT16_MAX>, true, DXGI_FORMAT_R16G16B16A16_UNORM, 8 },
        },
    },
    { // GL_INT
        { // unnormalized
            { &copyToFloatVertexData<GLint, 1, false>, false, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyToFloatVertexData<GLint, 2, false>, false, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyToFloatVertexData<GLint, 3, false>, false, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyToFloatVertexData<GLint, 4, false>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
        { // normalized
            { &copyToFloatVertexData<GLint, 1, true>, false, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyToFloatVertexData<GLint, 2, true>, false, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyToFloatVertexData<GLint, 3, true>, false, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyToFloatVertexData<GLint, 4, true>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
    },
    { // GL_UNSIGNED_INT
        { // unnormalized
            { &copyToFloatVertexData<GLuint, 1, false>, false, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyToFloatVertexData<GLuint, 2, false>, false, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyToFloatVertexData<GLuint, 3, false>, false, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyToFloatVertexData<GLuint, 4, false>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
        { // normalized
            { &copyToFloatVertexData<GLuint, 1, true>, false, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyToFloatVertexData<GLuint, 2, true>, false, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyToFloatVertexData<GLuint, 3, true>, false, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyToFloatVertexData<GLuint, 4, true>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
    },
    { // GL_INT_2_10_10_10_REV
        { // unnormalized
            { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
            { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
            { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
            { &copyPackedVertexData<true, false, true>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
        { // normalized
            { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
            { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
            { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
            { &copyPackedVertexData<true, true, true>, true, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
    },
    { // GL_UNSIGNED_INT_2_10_10_10_REV
        { // unnormalized
            { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
            { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
            { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
            { &copyPackedVertexData<true, false, true>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
        { // normalized
            { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
            { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
            { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
            { &copyPackedUnsignedVertexData, true, DXGI_FORMAT_R10G10B10A2_UNORM, 4 },
        },
    },
    { // GL_FIXED
        { // unnormalized
            { &copyFixedVertexData<1>, false, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyFixedVertexData<2>, false, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyFixedVertexData<3>, false, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyFixedVertexData<4>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
        { // normalized
            { &copyFixedVertexData<1>, false, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyFixedVertexData<2>, false, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyFixedVertexData<3>, false, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyFixedVertexData<4>, false, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
    },
    { // GL_HALF_FLOAT
        { // unnormalized
            { &copyVertexData<GLhalf, 1, false, gl::Float16One>, true, DXGI_FORMAT_R16_FLOAT, 2 },
            { &copyVertexData<GLhalf, 2, false, gl::Float16One>, true, DXGI_FORMAT_R16G16_FLOAT, 4 },
            { &copyVertexData<GLhalf, 3, true, gl::Float16One>, false, DXGI_FORMAT_R16G16B16A16_FLOAT, 8 },
            { &copyVertexData<GLhalf, 4, false, gl::Float16One>, true, DXGI_FORMAT_R16G16B16A16_FLOAT, 8 },
        },
        { // normalized
            { &copyVertexData<GLhalf, 1, false, gl::Float16One>, true, DXGI_FORMAT_R16_FLOAT, 2 },
            { &copyVertexData<GLhalf, 2, false, gl::Float16One>, true, DXGI_FORMAT_R16G16_FLOAT, 4 },
            { &copyVertexData<GLhalf, 3, true, gl::Float16One>, false, DXGI_FORMAT_R16G16B16A16_FLOAT, 8 },
            { &copyVertexData<GLhalf, 4, false, gl::Float16One>, true, DXGI_FORMAT_R16G16B16A16_FLOAT, 8 },
        },
    },
    { // GL_FLOAT
        { // unnormalized
            { &copyVertexData<GLfloat, 1, false, gl::Float32One>, true, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyVertexData<GLfloat, 2, false, gl::Float32One>, true, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyVertexData<GLfloat, 3, false, gl::Float32One>, true, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyVertexData<GLfloat, 4, false, gl::Float32One>, true, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
        { // normalized
            { &copyVertexData<GLfloat, 1, false, gl::Float32One>, true, DXGI_FORMAT_R32_FLOAT, 4 },
            { &copyVertexData<GLfloat, 2, false, gl::Float32One>, true, DXGI_FORMAT_R32G32_FLOAT, 8 },
            { &copyVertexData<GLfloat, 3, false, gl::Float32One>, true, DXGI_FORMAT_R32G32B32_FLOAT, 12 },
            { &copyVertexData<GLfloat, 4, false, gl::Float32One>, true, DXGI_FORMAT_R32G32B32A32_FLOAT, 16 },
        },
    },
};

const VertexBuffer11::VertexConverter VertexBuffer11::mIntegerVertexTranslations[NUM_GL_INTEGER_VERTEX_ATTRIB_TYPES][4] =
{
    { // GL_BYTE
        { &copyVertexData<GLbyte, 1, false, 1>, true, DXGI_FORMAT_R8_SINT, 1 },
        { &copyVertexData<GLbyte, 2, false, 1>, true, DXGI_FORMAT_R8G8_SINT, 2 },
        { &copyVertexData<GLbyte, 3, true, 1>, false, DXGI_FORMAT_R8G8B8A8_SINT, 4 },
        { &copyVertexData<GLbyte, 4, false, 1>, true, DXGI_FORMAT_R8G8B8A8_SINT, 4 },
    },
    { // GL_UNSIGNED_BYTE
        { &copyVertexData<GLubyte, 1, false, 1>, true, DXGI_FORMAT_R8_UINT, 1 },
        { &copyVertexData<GLubyte, 2, false, 1>, true, DXGI_FORMAT_R8G8_UINT, 2 },
        { &copyVertexData<GLubyte, 3, true, 1>, false, DXGI_FORMAT_R8G8B8A8_UINT, 4 },
        { &copyVertexData<GLubyte, 4, false, 1>, true, DXGI_FORMAT_R8G8B8A8_UINT, 4 },
    },
    { // GL_SHORT
        { &copyVertexData<GLshort, 1, false, 1>, true, DXGI_FORMAT_R16_SINT, 2 },
        { &copyVertexData<GLshort, 2, false, 1>, true, DXGI_FORMAT_R16G16_SINT, 4 },
        { &copyVertexData<GLshort, 3, true, 1>, false, DXGI_FORMAT_R16G16B16A16_SINT, 8 },
        { &copyVertexData<GLshort, 4, false, 1>, true, DXGI_FORMAT_R16G16B16A16_SINT, 8 },
    },
    { // GL_UNSIGNED_SHORT
        { &copyVertexData<GLushort, 1, false, 1>, true, DXGI_FORMAT_R16_UINT, 2 },
        { &copyVertexData<GLushort, 2, false, 1>, true, DXGI_FORMAT_R16G16_UINT, 4 },
        { &copyVertexData<GLushort, 3, true, 1>, false, DXGI_FORMAT_R16G16B16A16_UINT, 8 },
        { &copyVertexData<GLushort, 4, false, 1>, true, DXGI_FORMAT_R16G16B16A16_UINT, 8 },
    },
    { // GL_INT
        { &copyVertexData<GLint, 1, false, 1>, true, DXGI_FORMAT_R32_SINT, 4 },
        { &copyVertexData<GLint, 2, false, 1>, true, DXGI_FORMAT_R32G32_SINT, 8 },
        { &copyVertexData<GLint, 3, false, 1>, true, DXGI_FORMAT_R32G32B32_SINT, 12 },
        { &copyVertexData<GLint, 4, false, 1>, true, DXGI_FORMAT_R32G32B32A32_SINT, 16 },
    },
    { // GL_UNSIGNED_INT
        { &copyVertexData<GLuint, 1, false, 1>, true, DXGI_FORMAT_R32_UINT, 4 },
        { &copyVertexData<GLuint, 2, false, 1>, true, DXGI_FORMAT_R32G32_UINT, 8 },
        { &copyVertexData<GLuint, 3, false, 1>, true, DXGI_FORMAT_R32G32B32_UINT, 12 },
        { &copyVertexData<GLuint, 4, false, 1>, true, DXGI_FORMAT_R32G32B32A32_UINT, 16 },
    },
    { // GL_INT_2_10_10_10_REV
        { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
        { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
        { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
        { &copyPackedVertexData<true, true, false>, true, DXGI_FORMAT_R16G16B16A16_SINT, 8 },
    },
    { // GL_UNSIGNED_INT_2_10_10_10_REV
        { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
        { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
        { NULL, false, DXGI_FORMAT_UNKNOWN, 0 },
        { &copyPackedUnsignedVertexData, true, DXGI_FORMAT_R10G10B10A2_UINT, 4 },
    },
};

const VertexBuffer11::VertexConverter &VertexBuffer11::getVertexConversion(const gl::VertexAttribute &attribute)
{
    return getVertexConversion(attribute.mType, attribute.mPureInteger, attribute.mNormalized, attribute.mSize);
}

const VertexBuffer11::VertexConverter &VertexBuffer11::getVertexConversion(GLenum type, bool pureInteger, bool normalized, int size)
{
    if (pureInteger)
    {
        const unsigned int typeIndex = GetIntegerTypeIndex(type);
        return mIntegerVertexTranslations[typeIndex][size - 1];
    }
    else
    {
        const unsigned int typeIndex = GetFloatTypeIndex(type);
        return mFloatVertexTranslations[typeIndex][normalized ? 1 : 0][size - 1];
    }
}

}
