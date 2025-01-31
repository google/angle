//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// mtl_common.h:
//      Declares common constants, template classes, and mtl::Context - the MTLDevice container &
//      error handler base class.
//

#ifndef LIBANGLE_RENDERER_METAL_MTL_COMMON_H_
#define LIBANGLE_RENDERER_METAL_MTL_COMMON_H_

#import <Metal/Metal.h>

#include <TargetConditionals.h>

#include <string>

#include "common/Optional.h"
#include "common/PackedEnums.h"
#include "common/angleutils.h"
#include "common/apple_platform_utils.h"
#include "libANGLE/Constants.h"
#include "libANGLE/ImageIndex.h"
#include "libANGLE/Version.h"
#include "libANGLE/angletypes.h"

#if defined(ANGLE_MTL_ENABLE_TRACE)
#    define ANGLE_MTL_LOG(...) NSLog(@__VA_ARGS__)
#else
#    define ANGLE_MTL_LOG(...) (void)0
#endif

#define ANGLE_MTL_OBJC_SCOPE ANGLE_APPLE_OBJC_SCOPE
#define ANGLE_MTL_RETAIN ANGLE_APPLE_RETAIN
#define ANGLE_MTL_RELEASE ANGLE_APPLE_RELEASE

namespace egl
{
class Display;
class Image;
class Surface;
}  // namespace egl

#define ANGLE_GL_OBJECTS_X(PROC) \
    PROC(Buffer)                 \
    PROC(Context)                \
    PROC(Framebuffer)            \
    PROC(MemoryObject)           \
    PROC(Query)                  \
    PROC(Program)                \
    PROC(ProgramExecutable)      \
    PROC(Sampler)                \
    PROC(Semaphore)              \
    PROC(Texture)                \
    PROC(TransformFeedback)      \
    PROC(VertexArray)

#define ANGLE_PRE_DECLARE_OBJECT(OBJ) class OBJ;

namespace gl
{
ANGLE_GL_OBJECTS_X(ANGLE_PRE_DECLARE_OBJECT)
}  // namespace gl

#define ANGLE_PRE_DECLARE_MTL_OBJECT(OBJ) class OBJ##Mtl;

namespace rx
{
class DisplayMtl;
class ContextMtl;
class FramebufferMtl;
class BufferMtl;
class ImageMtl;
class VertexArrayMtl;
class TextureMtl;
class ProgramMtl;
class SamplerMtl;
class TransformFeedbackMtl;

ANGLE_GL_OBJECTS_X(ANGLE_PRE_DECLARE_MTL_OBJECT)

namespace mtl
{

// NOTE(hqle): support variable max number of vertex attributes
constexpr uint32_t kMaxVertexAttribs = gl::MAX_VERTEX_ATTRIBS;
// Note: This is the max number of render targets the backend supports.
// It is NOT how many the device supports which may be lower. If you
// increase this number you will also need to edit the shaders in
// metal/shaders/common.h.
constexpr uint32_t kMaxRenderTargets = 8;
// Metal Apple1 iOS devices only support 4 render targets
constexpr uint32_t kMaxRenderTargetsOlderGPUFamilies = 4;

constexpr uint32_t kMaxColorTargetBitsApple1To3      = 256;
constexpr uint32_t kMaxColorTargetBitsApple4Plus     = 512;
constexpr uint32_t kMaxColorTargetBitsMacAndCatalyst = std::numeric_limits<uint32_t>::max();

constexpr uint32_t kMaxShaderUBOs = 12;
constexpr uint32_t kMaxUBOSize    = 16384;

constexpr uint32_t kMaxShaderXFBs = gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS;

// The max size of a buffer that will be allocated in shared memory.
// NOTE(hqle): This is just a hint. There is no official document on what is the max allowed size
// for shared memory.
constexpr size_t kSharedMemBufferMaxBufSizeHint = 256 * 1024;

constexpr size_t kDefaultAttributeSize = 4 * sizeof(float);

// Metal limits
constexpr uint32_t kMaxShaderBuffers     = 31;
constexpr uint32_t kMaxShaderSamplers    = 16;
constexpr size_t kInlineConstDataMaxSize = 4 * 1024;
constexpr size_t kDefaultUniformsMaxSize = 16 * 1024;
constexpr uint32_t kMaxViewports         = 1;
constexpr uint32_t kMaxShaderImages      = gl::IMPLEMENTATION_MAX_PIXEL_LOCAL_STORAGE_PLANES;

// Restrict in-flight resource usage to 400 MB.
// A render pass can use more than 400MB, but the command buffer
// will be flushed next time
constexpr const size_t kMaximumResidentMemorySizeInBytes = 400 * 1024 * 1024;

// Restrict in-flight render passes per command buffer to 16.
// The goal is to reduce the number of active render passes on the system at
// any one time and this value was determined through experimentation.
constexpr uint32_t kMaxRenderPassesPerCommandBuffer = 16;

constexpr uint32_t kVertexAttribBufferStrideAlignment = 4;
// Alignment requirement for offset passed to setVertex|FragmentBuffer
#if TARGET_OS_OSX || TARGET_OS_MACCATALYST
constexpr uint32_t kUniformBufferSettingOffsetMinAlignment = 256;
#else
constexpr uint32_t kUniformBufferSettingOffsetMinAlignment = 4;
#endif
constexpr uint32_t kIndexBufferOffsetAlignment       = 4;
constexpr uint32_t kArgumentBufferOffsetAlignment    = kUniformBufferSettingOffsetMinAlignment;
constexpr uint32_t kTextureToBufferBlittingAlignment = 256;

// Front end binding limits
constexpr uint32_t kMaxGLSamplerBindings = 2 * kMaxShaderSamplers;
constexpr uint32_t kMaxGLUBOBindings     = 2 * kMaxShaderUBOs;

// Binding index start for vertex data buffers:
constexpr uint32_t kVboBindingIndexStart = 0;

// Binding index for default attribute buffer:
constexpr uint32_t kDefaultAttribsBindingIndex = kVboBindingIndexStart + kMaxVertexAttribs;
// Binding index for driver uniforms:
constexpr uint32_t kDriverUniformsBindingIndex = kDefaultAttribsBindingIndex + 1;
// Binding index for default uniforms:
constexpr uint32_t kDefaultUniformsBindingIndex = kDefaultAttribsBindingIndex + 3;
// Binding index for Transform Feedback Buffers (4)
constexpr uint32_t kTransformFeedbackBindingIndex = kDefaultUniformsBindingIndex + 1;
// Binding index for shadow samplers' compare modes
constexpr uint32_t kShadowSamplerCompareModesBindingIndex = kTransformFeedbackBindingIndex + 4;
// Binding index for UBO's argument buffer
constexpr uint32_t kUBOArgumentBufferBindingIndex = kShadowSamplerCompareModesBindingIndex + 1;

constexpr uint32_t kStencilMaskAll = 0xff;  // Only 8 bits stencil is supported

// This special constant is used to indicate that a particular vertex descriptor's buffer layout
// index is unused.
constexpr MTLVertexStepFunction kVertexStepFunctionInvalid =
    static_cast<MTLVertexStepFunction>(0xff);

constexpr int kEmulatedAlphaValue = 1;

constexpr size_t kOcclusionQueryResultSize = sizeof(uint64_t);

constexpr gl::Version kMaxSupportedGLVersion = gl::Version(3, 0);

enum class PixelType
{
    Int,
    UInt,
    Float,
    EnumCount,
};

template <typename T>
struct ImplTypeHelper;

// clang-format off
#define ANGLE_IMPL_TYPE_HELPER_GL(OBJ) \
template<>                             \
struct ImplTypeHelper<gl::OBJ>         \
{                                      \
    using ImplType = OBJ##Mtl;         \
};
// clang-format on

ANGLE_GL_OBJECTS_X(ANGLE_IMPL_TYPE_HELPER_GL)

template <>
struct ImplTypeHelper<egl::Display>
{
    using ImplType = DisplayMtl;
};

template <>
struct ImplTypeHelper<egl::Image>
{
    using ImplType = ImageMtl;
};

template <typename T>
using GetImplType = typename ImplTypeHelper<T>::ImplType;

template <typename T>
GetImplType<T> *GetImpl(const T *glObject)
{
    return GetImplAs<GetImplType<T>>(glObject);
}

// This class wraps Objective-C pointer inside, it will manage the lifetime of
// the Objective-C pointer. Changing pointer is not supported outside subclass.
template <typename T>
class WrappedObject
{
  public:
    WrappedObject() = default;
    ~WrappedObject() { release(); }

    bool valid() const { return (mMetalObject != nil); }

    T get() const { return mMetalObject; }
    T leakObject() { return std::exchange(mMetalObject, nullptr); }
    inline void reset() { release(); }

    operator T() const { return get(); }

  protected:
    inline void set(T obj) { retainAssign(obj); }

    void retainAssign(T obj)
    {

#if !__has_feature(objc_arc)
        T retained = obj;
        [retained retain];
#endif
        release();
        mMetalObject = obj;
    }

    void unretainAssign(T obj)
    {
        release();
        mMetalObject = obj;
    }

  private:
    void release()
    {
#if !__has_feature(objc_arc)
        [mMetalObject release];
#endif
        mMetalObject = nil;
    }

    T mMetalObject = nil;
};

// Smart pointer for holding Objective-C objects. Use adoptObjCPtr for create functions
// that return owned reference, e.g. functions that begin with 'new', 'copy', 'create'.
template <typename T>
class ObjCPtr
{
  public:
    using PtrType = std::remove_pointer_t<T> *;

    constexpr ObjCPtr() = default;
    constexpr ObjCPtr(std::nullptr_t other) {}
    ObjCPtr(PtrType other);
    ObjCPtr(const ObjCPtr &other);
    template <typename U>
    constexpr ObjCPtr(ObjCPtr<U> &&other);
    ~ObjCPtr();
    ObjCPtr &operator=(const ObjCPtr &other);
    ObjCPtr &operator=(PtrType other);
    template <typename U>
    constexpr ObjCPtr &operator=(ObjCPtr<U> &&other);

    [[nodiscard]] constexpr PtrType leakObject();
    void reset();

    constexpr explicit operator bool() const { return get(); }
    constexpr operator PtrType() const { return get(); }
    constexpr PtrType get() const { return mObject; }
    constexpr void swap(ObjCPtr<T> &other);

    template <typename U>
    friend ObjCPtr<std::remove_pointer_t<U>> adoptObjCPtr(U NS_RELEASES_ARGUMENT);

  private:
    struct AdoptTag
    {};
    constexpr ObjCPtr(PtrType other, AdoptTag);

    PtrType mObject = nil;
};

template <typename T>
ObjCPtr(T) -> ObjCPtr<std::remove_pointer_t<T>>;

template <typename T>
ObjCPtr<T>::ObjCPtr(PtrType other) : mObject(other)
{
#if !__has_feature(objc_arc)
    [mObject retain];
#endif
}

template <typename T>
ObjCPtr<T>::ObjCPtr(const ObjCPtr &other) : ObjCPtr(other.mObject)
{}

template <typename T>
template <typename U>
constexpr ObjCPtr<T>::ObjCPtr(ObjCPtr<U> &&other) : mObject(other.leakObject())
{}

template <typename T>
ObjCPtr<T>::~ObjCPtr()
{
#if !__has_feature(objc_arc)
    [mObject release];
#endif
}

template <typename T>
ObjCPtr<T> &ObjCPtr<T>::operator=(const ObjCPtr &other)
{
    ObjCPtr temp = other;
    swap(temp);
    return *this;
}

template <typename T>
template <typename U>
constexpr ObjCPtr<T> &ObjCPtr<T>::operator=(ObjCPtr<U> &&other)
{
    ObjCPtr temp = std::move(other);
    swap(temp);
    return *this;
}

template <typename T>
ObjCPtr<T> &ObjCPtr<T>::operator=(PtrType other)
{
    ObjCPtr temp = other;
    swap(temp);
    return *this;
}

template <typename T>
constexpr ObjCPtr<T>::ObjCPtr(PtrType other, AdoptTag) : mObject(other)
{}

template <typename T>
constexpr void ObjCPtr<T>::swap(ObjCPtr<T> &other)
{
    // std::swap is constexpr only in c++20.
    auto object   = other.mObject;
    other.mObject = mObject;
    mObject       = object;
}

template <typename T>
constexpr typename ObjCPtr<T>::PtrType ObjCPtr<T>::leakObject()
{
    // std::exchange is constexper only in c++20.
    auto object = mObject;
    mObject     = nullptr;
    return object;
}

template <typename T>
void ObjCPtr<T>::reset()
{
    *this = {};
}

template <typename T, typename U>
constexpr bool operator==(const ObjCPtr<T> &a, const ObjCPtr<U> &b)
{
    return a.get() == b.get();
}

template <typename T, typename U>
constexpr bool operator==(const ObjCPtr<T> &a, U *b)
{
    return a.get() == b;
}

template <typename T, typename U>
constexpr bool operator==(T *a, const ObjCPtr<U> &b)
{
    return a == b.get();
}

template <typename U>
ObjCPtr<std::remove_pointer_t<U>> adoptObjCPtr(U NS_RELEASES_ARGUMENT other)
{
    using ResultType = ObjCPtr<std::remove_pointer_t<U>>;
    return ResultType(other, typename ResultType::AdoptTag{});
}

// The native image index used by Metal back-end,  the image index uses native mipmap level instead
// of "virtual" level modified by OpenGL's base level.
using MipmapNativeLevel = gl::LevelIndexWrapper<uint32_t>;

constexpr MipmapNativeLevel kZeroNativeMipLevel(0);

class ImageNativeIndexIterator;

class ImageNativeIndex final
{
  public:
    ImageNativeIndex() = delete;
    ImageNativeIndex(const gl::ImageIndex &src, GLint baseLevel)
    {
        mNativeIndex = gl::ImageIndex::MakeFromType(src.getType(), src.getLevelIndex() - baseLevel,
                                                    src.getLayerIndex(), src.getLayerCount());
    }

    static ImageNativeIndex FromBaseZeroGLIndex(const gl::ImageIndex &src)
    {
        return ImageNativeIndex(src, 0);
    }

    MipmapNativeLevel getNativeLevel() const
    {
        return MipmapNativeLevel(mNativeIndex.getLevelIndex());
    }

    gl::TextureType getType() const { return mNativeIndex.getType(); }
    GLint getLayerIndex() const { return mNativeIndex.getLayerIndex(); }
    GLint getLayerCount() const { return mNativeIndex.getLayerCount(); }
    GLint cubeMapFaceIndex() const { return mNativeIndex.cubeMapFaceIndex(); }

    bool isLayered() const { return mNativeIndex.isLayered(); }
    bool hasLayer() const { return mNativeIndex.hasLayer(); }
    bool has3DLayer() const { return mNativeIndex.has3DLayer(); }
    bool usesTex3D() const { return mNativeIndex.usesTex3D(); }

    bool valid() const { return mNativeIndex.valid(); }

    ImageNativeIndexIterator getLayerIterator(GLint layerCount) const;

  private:
    gl::ImageIndex mNativeIndex;
};

class ImageNativeIndexIterator final
{
  public:
    ImageNativeIndex next() { return ImageNativeIndex(mNativeIndexIte.next(), 0); }
    ImageNativeIndex current() const { return ImageNativeIndex(mNativeIndexIte.current(), 0); }
    bool hasNext() const { return mNativeIndexIte.hasNext(); }

  private:
    // This class is only constructable from ImageNativeIndex
    friend class ImageNativeIndex;

    explicit ImageNativeIndexIterator(const gl::ImageIndexIterator &baseZeroSrc)
        : mNativeIndexIte(baseZeroSrc)
    {}

    gl::ImageIndexIterator mNativeIndexIte;
};

using ClearColorValueBytes = std::array<uint8_t, 4 * sizeof(float)>;

class ClearColorValue
{
  public:
    constexpr ClearColorValue()
        : mType(PixelType::Float), mRedF(0), mGreenF(0), mBlueF(0), mAlphaF(0)
    {}
    constexpr ClearColorValue(float r, float g, float b, float a)
        : mType(PixelType::Float), mRedF(r), mGreenF(g), mBlueF(b), mAlphaF(a)
    {}
    constexpr ClearColorValue(int32_t r, int32_t g, int32_t b, int32_t a)
        : mType(PixelType::Int), mRedI(r), mGreenI(g), mBlueI(b), mAlphaI(a)
    {}
    constexpr ClearColorValue(uint32_t r, uint32_t g, uint32_t b, uint32_t a)
        : mType(PixelType::UInt), mRedU(r), mGreenU(g), mBlueU(b), mAlphaU(a)
    {}
    constexpr ClearColorValue(const ClearColorValue &src)
        : mType(src.mType), mValueBytes(src.mValueBytes)
    {}

    MTLClearColor toMTLClearColor() const;

    PixelType getType() const { return mType; }

    const ClearColorValueBytes &getValueBytes() const { return mValueBytes; }

    ClearColorValue &operator=(const ClearColorValue &src);

    void setAsFloat(float r, float g, float b, float a);
    void setAsInt(int32_t r, int32_t g, int32_t b, int32_t a);
    void setAsUInt(uint32_t r, uint32_t g, uint32_t b, uint32_t a);

  private:
    PixelType mType;

    union
    {
        struct
        {
            float mRedF, mGreenF, mBlueF, mAlphaF;
        };
        struct
        {
            int32_t mRedI, mGreenI, mBlueI, mAlphaI;
        };
        struct
        {
            uint32_t mRedU, mGreenU, mBlueU, mAlphaU;
        };

        ClearColorValueBytes mValueBytes;
    };
};

class CommandQueue;

class ErrorHandler
{
  public:
    virtual ~ErrorHandler() {}

    virtual void handleError(GLenum error,
                             const char *message,
                             const char *file,
                             const char *function,
                             unsigned int line) = 0;

    void handleNSError(NSError *error, const char *file, const char *function, unsigned int line)
    {
        std::string message;
        {
            std::stringstream s;
            s << "Internal error. Metal error: "
              << (error != nil ? error.localizedDescription.UTF8String : "nil error");
            message = s.str();
        }
        handleError(GL_INVALID_OPERATION, message.c_str(), file, function, line);
    }
};

class Context : public ErrorHandler
{
  public:
    Context(DisplayMtl *displayMtl);
    mtl::CommandQueue &cmdQueue();

    DisplayMtl *getDisplay() const { return mDisplay; }

  protected:
    DisplayMtl *mDisplay;
};

#define ANGLE_MTL_CHECK(context, result, nserror)                                   \
    do                                                                              \
    {                                                                               \
        auto &localResult = (result);                                               \
        auto &localError  = (nserror);                                              \
        if (ANGLE_UNLIKELY(!localResult || localError))                             \
        {                                                                           \
            context->handleNSError(localError, __FILE__, ANGLE_FUNCTION, __LINE__); \
            return angle::Result::Stop;                                             \
        }                                                                           \
    } while (0)

}  // namespace mtl
}  // namespace rx

#endif /* LIBANGLE_RENDERER_METAL_MTL_COMMON_H_ */
