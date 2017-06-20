//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// angleutils.h: Common ANGLE utilities.

#ifndef COMMON_ANGLEUTILS_H_
#define COMMON_ANGLEUTILS_H_

#include "common/platform.h"

#include <climits>
#include <cstdarg>
#include <cstddef>
#include <string>
#include <set>
#include <sstream>
#include <vector>

// A helper class to disallow copy and assignment operators
namespace angle
{

#if defined(ANGLE_ENABLE_D3D9) || defined(ANGLE_ENABLE_D3D11)
using Microsoft::WRL::ComPtr;
#endif  // defined(ANGLE_ENABLE_D3D9) || defined(ANGLE_ENABLE_D3D11)

class NonCopyable
{
  protected:
    NonCopyable() = default;
    ~NonCopyable() = default;

  private:
    NonCopyable(const NonCopyable&) = delete;
    void operator=(const NonCopyable&) = delete;
};

extern const uintptr_t DirtyPointer;

// Helper class for wrapping an onDestroy function.
template <typename ObjT, typename ContextT>
class UniqueObjectPointer : angle::NonCopyable
{
  public:
    UniqueObjectPointer(const ContextT *context) : mObject(nullptr), mContext(context) {}
    UniqueObjectPointer(ObjT *obj, const ContextT *context) : mObject(obj), mContext(context) {}
    ~UniqueObjectPointer()
    {
        if (mObject)
        {
            mObject->onDestroy(mContext);
        }
    }

    ObjT *operator->() const { return mObject; }

    ObjT *release()
    {
        auto obj = mObject;
        mObject  = nullptr;
        return obj;
    }

    ObjT *get() const { return mObject; }

    void reset(ObjT *obj)
    {
        if (mObject)
        {
            mObject->onDestroy(mContext);
        }
        mObject = obj;
    }

  private:
    ObjT *mObject;
    const ContextT *mContext;
};

}  // namespace angle

template <typename T, size_t N>
constexpr inline size_t ArraySize(T (&)[N])
{
    return N;
}

template <typename T>
class WrappedArray final : angle::NonCopyable
{
  public:
    template <size_t N>
    constexpr WrappedArray(const T (&data)[N]) : mArray(&data[0]), mSize(N)
    {
    }

    constexpr WrappedArray() : mArray(nullptr), mSize(0) {}
    constexpr WrappedArray(const T *data, size_t size) : mArray(data), mSize(size) {}
    ~WrappedArray() {}

    constexpr const T *get() const { return mArray; }
    constexpr size_t size() const { return mSize; }

  private:
    const T *mArray;
    size_t mSize;
};

template <typename T, unsigned int N>
void SafeRelease(T (&resourceBlock)[N])
{
    for (unsigned int i = 0; i < N; i++)
    {
        SafeRelease(resourceBlock[i]);
    }
}

template <typename T>
void SafeRelease(T& resource)
{
    if (resource)
    {
        resource->Release();
        resource = nullptr;
    }
}

template <typename T>
void SafeDelete(T *&resource)
{
    delete resource;
    resource = nullptr;
}

template <typename T>
void SafeDeleteContainer(T& resource)
{
    for (auto &element : resource)
    {
        SafeDelete(element);
    }
    resource.clear();
}

template <typename T>
void SafeDeleteArray(T*& resource)
{
    delete[] resource;
    resource = nullptr;
}

// Provide a less-than function for comparing structs
// Note: struct memory must be initialized to zero, because of packing gaps
template <typename T>
inline bool StructLessThan(const T &a, const T &b)
{
    return (memcmp(&a, &b, sizeof(T)) < 0);
}

// Provide a less-than function for comparing structs
// Note: struct memory must be initialized to zero, because of packing gaps
template <typename T>
inline bool StructEquals(const T &a, const T &b)
{
    return (memcmp(&a, &b, sizeof(T)) == 0);
}

template <typename T>
inline void StructZero(T *obj)
{
    memset(obj, 0, sizeof(T));
}

template <typename T>
inline bool IsMaskFlagSet(T mask, T flag)
{
    // Handles multibit flags as well
    return (mask & flag) == flag;
}

inline const char* MakeStaticString(const std::string &str)
{
    static std::set<std::string> strings;
    std::set<std::string>::iterator it = strings.find(str);
    if (it != strings.end())
    {
        return it->c_str();
    }

    return strings.insert(str).first->c_str();
}

inline std::string ArrayString(unsigned int i)
{
    // We assume UINT_MAX and GL_INVALID_INDEX are equal
    // See DynamicHLSL.cpp
    if (i == UINT_MAX)
    {
        return "";
    }

    std::stringstream strstr;

    strstr << "[";
    strstr << i;
    strstr << "]";

    return strstr.str();
}

inline std::string Str(int i)
{
    std::stringstream strstr;
    strstr << i;
    return strstr.str();
}

size_t FormatStringIntoVector(const char *fmt, va_list vararg, std::vector<char>& buffer);

std::string FormatString(const char *fmt, va_list vararg);
std::string FormatString(const char *fmt, ...);

template <typename T>
std::string ToString(const T &value)
{
    std::ostringstream o;
    o << value;
    return o.str();
}

// snprintf is not defined with MSVC prior to to msvc14
#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

#define GL_BGR565_ANGLEX 0x6ABB
#define GL_BGRA4_ANGLEX 0x6ABC
#define GL_BGR5_A1_ANGLEX 0x6ABD
#define GL_INT_64_ANGLEX 0x6ABE
#define GL_STRUCT_ANGLEX 0x6ABF

// Hidden enum for the NULL D3D device type.
#define EGL_PLATFORM_ANGLE_DEVICE_TYPE_NULL_ANGLE 0x6AC0

// TODO(jmadill): Clean this up at some point.
#define EGL_PLATFORM_ANGLE_PLATFORM_METHODS_ANGLEX 0x9999

#define ANGLE_TRY_CHECKED_MATH(result)                     \
    if (!result.IsValid())                                 \
    {                                                      \
        return gl::InternalError() << "Integer overflow."; \
    }

#endif // COMMON_ANGLEUTILS_H_
