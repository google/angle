//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// params:
//   Parameter wrapper structs for OpenGL ES. These helpers cache re-used values
//   in entry point routines.

#ifndef LIBANGLE_PARAMS_H_
#define LIBANGLE_PARAMS_H_

#include "angle_gl.h"
#include "common/Optional.h"
#include "common/angleutils.h"
#include "common/mathutil.h"

namespace gl
{
class Context;

enum class EntryPoint
{
    Invalid,
    DrawArrays,
    DrawElements,
    DrawElementsInstanced,
    DrawElementsInstancedANGLE,
    DrawRangeElements,
};

template <EntryPoint EP>
struct EntryPointParam;

template <EntryPoint EP>
using EntryPointParamType = typename EntryPointParam<EP>::Type;

class ParamTypeInfo
{
  public:
    constexpr ParamTypeInfo(const char *selfClass, const ParamTypeInfo *parentType)
        : mSelfClass(selfClass), mParentTypeInfo(parentType)
    {
    }

    constexpr bool hasDynamicType(const ParamTypeInfo &typeInfo) const
    {
        return mSelfClass == typeInfo.mSelfClass ||
               (mParentTypeInfo && mParentTypeInfo->hasDynamicType(typeInfo));
    }

    constexpr bool isValid() const { return mSelfClass != nullptr; }

  private:
    const char *mSelfClass;
    const ParamTypeInfo *mParentTypeInfo;
};

#define ANGLE_PARAM_TYPE_INFO(NAME, BASENAME) \
    static constexpr ParamTypeInfo TypeInfo = {#NAME, &BASENAME::TypeInfo}

class ParamsBase : angle::NonCopyable
{
  public:
    ParamsBase(Context *context, ...);

    template <EntryPoint EP, typename... ArgsT>
    static void Factory(EntryPointParamType<EP> *objBuffer, ArgsT... args);

#if defined(ANGLE_ENABLE_ASSERTS)
    static constexpr ParamTypeInfo TypeInfo = {nullptr, nullptr};
#endif  // defined(ANGLE_ENABLE_ASSERTS)
};

// static
template <EntryPoint EP, typename... ArgsT>
void ParamsBase::Factory(EntryPointParamType<EP> *objBuffer, ArgsT... args)
{
    new (objBuffer) EntryPointParamType<EP>(args...);
}

class HasIndexRange : public ParamsBase
{
  public:
    HasIndexRange(Context *context, GLsizei count, GLenum type, const void *indices);

    template <EntryPoint EP, typename... ArgsT>
    static void Factory(HasIndexRange *objBuffer, ArgsT... args);

    const Optional<IndexRange> &getIndexRange() const;

    ANGLE_PARAM_TYPE_INFO(HasIndexRange, ParamsBase);

  private:
    Context *mContext;
    GLsizei mCount;
    GLenum mType;
    const GLvoid *mIndices;
    mutable Optional<IndexRange> mIndexRange;
};

// Entry point funcs essentially re-map different entry point parameter arrays into
// the format the parameter type class expects. For example, for HasIndexRange, for the
// various indexed draw calls, they drop parameters that aren't useful and re-arrange
// the rest.
#define ANGLE_ENTRY_POINT_FUNC(NAME, CLASS, ...)    \
    \
template<> struct EntryPointParam<EntryPoint::NAME> \
    {                                               \
        using Type = CLASS;                         \
    };                                              \
    \
template<> inline void CLASS::Factory<EntryPoint::NAME>(__VA_ARGS__)

ANGLE_ENTRY_POINT_FUNC(DrawElements,
                       HasIndexRange,
                       HasIndexRange *objBuffer,
                       Context *context,
                       GLenum /*mode*/,
                       GLsizei count,
                       GLenum type,
                       const void *indices)
{
    return ParamsBase::Factory<EntryPoint::DrawElements>(objBuffer, context, count, type, indices);
}

ANGLE_ENTRY_POINT_FUNC(DrawElementsInstanced,
                       HasIndexRange,
                       HasIndexRange *objBuffer,
                       Context *context,
                       GLenum /*mode*/,
                       GLsizei count,
                       GLenum type,
                       const void *indices,
                       GLsizei /*instanceCount*/)
{
    return ParamsBase::Factory<EntryPoint::DrawElementsInstanced>(objBuffer, context, count, type,
                                                                  indices);
}

ANGLE_ENTRY_POINT_FUNC(DrawElementsInstancedANGLE,
                       HasIndexRange,
                       HasIndexRange *objBuffer,
                       Context *context,
                       GLenum /*mode*/,
                       GLsizei count,
                       GLenum type,
                       const void *indices,
                       GLsizei /*instanceCount*/)
{
    return ParamsBase::Factory<EntryPoint::DrawElementsInstancedANGLE>(objBuffer, context, count,
                                                                       type, indices);
}

ANGLE_ENTRY_POINT_FUNC(DrawRangeElements,
                       HasIndexRange,
                       HasIndexRange *objBuffer,
                       Context *context,
                       GLenum /*mode*/,
                       GLuint /*start*/,
                       GLuint /*end*/,
                       GLsizei count,
                       GLenum type,
                       const void *indices)
{
    return ParamsBase::Factory<EntryPoint::DrawRangeElements>(objBuffer, context, count, type,
                                                              indices);
}

#undef ANGLE_ENTRY_POINT_FUNC

template <EntryPoint EP>
struct EntryPointParam
{
    using Type = ParamsBase;
};

}  // namespace gl

#endif  // LIBANGLE_PARAMS_H_
