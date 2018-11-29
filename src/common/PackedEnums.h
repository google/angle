// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// PackedGLEnums_autogen.h:
//   Declares ANGLE-specific enums classes for GLEnum and functions operating
//   on them.

#ifndef COMMON_PACKEDGLENUMS_H_
#define COMMON_PACKEDGLENUMS_H_

#include "common/PackedEGLEnums_autogen.h"
#include "common/PackedGLEnums_autogen.h"

#include <array>
#include <bitset>
#include <cstddef>

#include <EGL/egl.h>

#include "common/bitset_utils.h"

namespace angle
{

// Return the number of elements of a packed enum, including the InvalidEnum element.
template <typename E>
constexpr size_t EnumSize()
{
    using UnderlyingType = typename std::underlying_type<E>::type;
    return static_cast<UnderlyingType>(E::EnumCount);
}

// Implementation of AllEnums which allows iterating over all the possible values for a packed enums
// like so:
//     for (auto value : AllEnums<MyPackedEnum>()) {
//         // Do something with the enum.
//     }

template <typename E>
class EnumIterator final
{
  private:
    using UnderlyingType = typename std::underlying_type<E>::type;

  public:
    EnumIterator(E value) : mValue(static_cast<UnderlyingType>(value)) {}
    EnumIterator &operator++()
    {
        mValue++;
        return *this;
    }
    bool operator==(const EnumIterator &other) const { return mValue == other.mValue; }
    bool operator!=(const EnumIterator &other) const { return mValue != other.mValue; }
    E operator*() const { return static_cast<E>(mValue); }

  private:
    UnderlyingType mValue;
};

template <typename E>
struct AllEnums
{
    EnumIterator<E> begin() const { return {static_cast<E>(0)}; }
    EnumIterator<E> end() const { return {E::InvalidEnum}; }
};

// PackedEnumMap<E, T> is like an std::array<T, E::EnumCount> but is indexed with enum values. It
// implements all of the std::array interface except with enum values instead of indices.
template <typename E, typename T, size_t MaxSize = EnumSize<E>()>
class PackedEnumMap
{
    using UnderlyingType = typename std::underlying_type<E>::type;
    using Storage        = std::array<T, MaxSize>;

  public:
    using InitPair = std::pair<E, T>;

    constexpr PackedEnumMap() = default;

    constexpr PackedEnumMap(std::initializer_list<InitPair> init) : mPrivateData{}
    {
        // We use a for loop instead of range-for to work around a limitation in MSVC.
        for (const InitPair *it = init.begin(); it != init.end(); ++it)
        {
            // This horrible const_cast pattern is necessary to work around a constexpr limitation.
            // See https://stackoverflow.com/q/34199774/ . Note that it should be fixed with C++17.
            const_cast<T &>(const_cast<const Storage &>(
                mPrivateData)[static_cast<UnderlyingType>(it->first)]) = it->second;
        }
    }

    // types:
    using value_type      = T;
    using pointer         = T *;
    using const_pointer   = const T *;
    using reference       = T &;
    using const_reference = const T &;

    using size_type       = size_t;
    using difference_type = ptrdiff_t;

    using iterator               = typename Storage::iterator;
    using const_iterator         = typename Storage::const_iterator;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // No explicit construct/copy/destroy for aggregate type
    void fill(const T &u) { mPrivateData.fill(u); }
    void swap(PackedEnumMap<E, T> &a) noexcept { mPrivateData.swap(a.mPrivateData); }

    // iterators:
    iterator begin() noexcept { return mPrivateData.begin(); }
    const_iterator begin() const noexcept { return mPrivateData.begin(); }
    iterator end() noexcept { return mPrivateData.end(); }
    const_iterator end() const noexcept { return mPrivateData.end(); }

    reverse_iterator rbegin() noexcept { return mPrivateData.rbegin(); }
    const_reverse_iterator rbegin() const noexcept { return mPrivateData.rbegin(); }
    reverse_iterator rend() noexcept { return mPrivateData.rend(); }
    const_reverse_iterator rend() const noexcept { return mPrivateData.rend(); }

    // capacity:
    constexpr size_type size() const noexcept { return mPrivateData.size(); }
    constexpr size_type max_size() const noexcept { return mPrivateData.max_size(); }
    constexpr bool empty() const noexcept { return mPrivateData.empty(); }

    // element access:
    reference operator[](E n)
    {
        ASSERT(static_cast<size_t>(n) < mPrivateData.size());
        return mPrivateData[static_cast<UnderlyingType>(n)];
    }

    constexpr const_reference operator[](E n) const
    {
        ASSERT(static_cast<size_t>(n) < mPrivateData.size());
        return mPrivateData[static_cast<UnderlyingType>(n)];
    }

    const_reference at(E n) const { return mPrivateData.at(static_cast<UnderlyingType>(n)); }
    reference at(E n) { return mPrivateData.at(static_cast<UnderlyingType>(n)); }

    reference front() { return mPrivateData.front(); }
    const_reference front() const { return mPrivateData.front(); }
    reference back() { return mPrivateData.back(); }
    const_reference back() const { return mPrivateData.back(); }

    T *data() noexcept { return mPrivateData.data(); }
    const T *data() const noexcept { return mPrivateData.data(); }

  private:
    Storage mPrivateData;
};

// PackedEnumBitSetE> is like an std::bitset<E::EnumCount> but is indexed with enum values. It
// implements the std::bitset interface except with enum values instead of indices.
template <typename E, typename DataT = uint32_t>
using PackedEnumBitSet = BitSetT<EnumSize<E>(), DataT, E>;

}  // namespace angle

namespace gl
{

TextureType TextureTargetToType(TextureTarget target);
TextureTarget NonCubeTextureTypeToTarget(TextureType type);

TextureTarget CubeFaceIndexToTextureTarget(size_t face);
size_t CubeMapTextureTargetToFaceIndex(TextureTarget target);
bool IsCubeMapFaceTarget(TextureTarget target);

constexpr TextureTarget kCubeMapTextureTargetMin = TextureTarget::CubeMapPositiveX;
constexpr TextureTarget kCubeMapTextureTargetMax = TextureTarget::CubeMapNegativeZ;
constexpr TextureTarget kAfterCubeMapTextureTargetMax =
    static_cast<TextureTarget>(static_cast<uint8_t>(kCubeMapTextureTargetMax) + 1);
struct AllCubeFaceTextureTargets
{
    angle::EnumIterator<TextureTarget> begin() const { return kCubeMapTextureTargetMin; }
    angle::EnumIterator<TextureTarget> end() const { return kAfterCubeMapTextureTargetMax; }
};

constexpr ShaderType kGLES2ShaderTypeMin = ShaderType::Vertex;
constexpr ShaderType kGLES2ShaderTypeMax = ShaderType::Fragment;
constexpr ShaderType kAfterGLES2ShaderTypeMax =
    static_cast<ShaderType>(static_cast<uint8_t>(kGLES2ShaderTypeMax) + 1);
struct AllGLES2ShaderTypes
{
    angle::EnumIterator<ShaderType> begin() const { return kGLES2ShaderTypeMin; }
    angle::EnumIterator<ShaderType> end() const { return kAfterGLES2ShaderTypeMax; }
};

constexpr ShaderType kShaderTypeMin = ShaderType::Vertex;
constexpr ShaderType kShaderTypeMax = ShaderType::Compute;
constexpr ShaderType kAfterShaderTypeMax =
    static_cast<ShaderType>(static_cast<uint8_t>(kShaderTypeMax) + 1);
struct AllShaderTypes
{
    angle::EnumIterator<ShaderType> begin() const { return kShaderTypeMin; }
    angle::EnumIterator<ShaderType> end() const { return kAfterShaderTypeMax; }
};

constexpr size_t kGraphicsShaderCount = static_cast<size_t>(ShaderType::EnumCount) - 1u;
// Arrange the shader types in the order of rendering pipeline
constexpr std::array<ShaderType, kGraphicsShaderCount> kAllGraphicsShaderTypes = {
    ShaderType::Vertex, ShaderType::Geometry, ShaderType::Fragment};

using ShaderBitSet = angle::PackedEnumBitSet<ShaderType, uint8_t>;
static_assert(sizeof(ShaderBitSet) == sizeof(uint8_t), "Unexpected size");

template <typename T>
using ShaderMap = angle::PackedEnumMap<ShaderType, T>;

TextureType SamplerTypeToTextureType(GLenum samplerType);

bool IsMultisampled(gl::TextureType type);

enum class PrimitiveMode : uint8_t
{
    Points                 = 0x0,
    Lines                  = 0x1,
    LineLoop               = 0x2,
    LineStrip              = 0x3,
    Triangles              = 0x4,
    TriangleStrip          = 0x5,
    TriangleFan            = 0x6,
    Unused1                = 0x7,
    Unused2                = 0x8,
    Unused3                = 0x9,
    LinesAdjacency         = 0xA,
    LineStripAdjacency     = 0xB,
    TrianglesAdjacency     = 0xC,
    TriangleStripAdjacency = 0xD,

    InvalidEnum = 0xE,
    EnumCount   = 0xE,
};

template <>
constexpr PrimitiveMode FromGLenum<PrimitiveMode>(GLenum from)
{
    if (from >= static_cast<GLenum>(PrimitiveMode::EnumCount))
    {
        return PrimitiveMode::InvalidEnum;
    }

    return static_cast<PrimitiveMode>(from);
}

constexpr GLenum ToGLenum(PrimitiveMode from)
{
    return static_cast<GLenum>(from);
}

static_assert(ToGLenum(PrimitiveMode::Points) == GL_POINTS, "PrimitiveMode violation");
static_assert(ToGLenum(PrimitiveMode::Lines) == GL_LINES, "PrimitiveMode violation");
static_assert(ToGLenum(PrimitiveMode::LineLoop) == GL_LINE_LOOP, "PrimitiveMode violation");
static_assert(ToGLenum(PrimitiveMode::LineStrip) == GL_LINE_STRIP, "PrimitiveMode violation");
static_assert(ToGLenum(PrimitiveMode::Triangles) == GL_TRIANGLES, "PrimitiveMode violation");
static_assert(ToGLenum(PrimitiveMode::TriangleStrip) == GL_TRIANGLE_STRIP,
              "PrimitiveMode violation");
static_assert(ToGLenum(PrimitiveMode::TriangleFan) == GL_TRIANGLE_FAN, "PrimitiveMode violation");
static_assert(ToGLenum(PrimitiveMode::LinesAdjacency) == GL_LINES_ADJACENCY,
              "PrimitiveMode violation");
static_assert(ToGLenum(PrimitiveMode::LineStripAdjacency) == GL_LINE_STRIP_ADJACENCY,
              "PrimitiveMode violation");
static_assert(ToGLenum(PrimitiveMode::TrianglesAdjacency) == GL_TRIANGLES_ADJACENCY,
              "PrimitiveMode violation");
static_assert(ToGLenum(PrimitiveMode::TriangleStripAdjacency) == GL_TRIANGLE_STRIP_ADJACENCY,
              "PrimitiveMode violation");

enum class DrawElementsType : size_t
{
    UnsignedByte  = 0,
    UnsignedShort = 1,
    UnsignedInt   = 2,
    InvalidEnum   = 3,
    EnumCount     = 3,
};

template <>
constexpr DrawElementsType FromGLenum<DrawElementsType>(GLenum from)
{
    GLenum scaled = (from - GL_UNSIGNED_BYTE);
    GLenum packed = (scaled >> 1);

    if ((scaled & 1) != 0 || packed >= static_cast<GLenum>(DrawElementsType::EnumCount))
    {
        return DrawElementsType::InvalidEnum;
    }

    return static_cast<DrawElementsType>(packed);
}

constexpr GLenum ToGLenum(DrawElementsType from)
{
    return ((static_cast<GLenum>(from) << 1) + GL_UNSIGNED_BYTE);
}

static_assert(ToGLenum(DrawElementsType::UnsignedByte) == GL_UNSIGNED_BYTE,
              "DrawElementsType violation");
static_assert(ToGLenum(DrawElementsType::UnsignedShort) == GL_UNSIGNED_SHORT,
              "DrawElementsType violation");
static_assert(ToGLenum(DrawElementsType::UnsignedInt) == GL_UNSIGNED_INT,
              "DrawElementsType violation");
static_assert(FromGLenum<DrawElementsType>(GL_UNSIGNED_BYTE) == DrawElementsType::UnsignedByte,
              "DrawElementsType violation");
static_assert(FromGLenum<DrawElementsType>(GL_UNSIGNED_SHORT) == DrawElementsType::UnsignedShort,
              "DrawElementsType violation");
static_assert(FromGLenum<DrawElementsType>(GL_UNSIGNED_INT) == DrawElementsType::UnsignedInt,
              "DrawElementsType violation");
}  // namespace gl

namespace egl
{
MessageType ErrorCodeToMessageType(EGLint errorCode);
}  // namespace egl

namespace egl_gl
{
gl::TextureTarget EGLCubeMapTargetToCubeMapTarget(EGLenum eglTarget);
gl::TextureTarget EGLImageTargetToTextureTarget(EGLenum eglTarget);
gl::TextureType EGLTextureTargetToTextureType(EGLenum eglTarget);
}  // namespace egl_gl

#endif  // COMMON_PACKEDGLENUMS_H_
