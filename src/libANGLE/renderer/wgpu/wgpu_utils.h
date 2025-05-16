//
// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef LIBANGLE_RENDERER_WGPU_WGPU_UTILS_H_
#define LIBANGLE_RENDERER_WGPU_WGPU_UTILS_H_

#include <stdint.h>
#include <webgpu/webgpu.h>
#include <climits>

#include "libANGLE/Caps.h"
#include "libANGLE/Error.h"
#include "libANGLE/angletypes.h"

#define ANGLE_WGPU_TRY(context, command)                                                     \
    do                                                                                       \
    {                                                                                        \
        auto ANGLE_LOCAL_VAR = command;                                                      \
        if (ANGLE_UNLIKELY(::rx::webgpu::IsWgpuError(ANGLE_LOCAL_VAR)))                      \
        {                                                                                    \
            (context)->handleError(GL_INVALID_OPERATION, "Internal WebGPU error.", __FILE__, \
                                   ANGLE_FUNCTION, __LINE__);                                \
            return angle::Result::Stop;                                                      \
        }                                                                                    \
    } while (0)

#define ANGLE_WGPU_BEGIN_DEBUG_ERROR_SCOPE(context)                             \
    ::rx::webgpu::DebugErrorScope(context->getInstance(), context->getDevice(), \
                                  WGPUErrorFilter_Validation)
#define ANGLE_WGPU_END_DEBUG_ERROR_SCOPE(context, scope) \
    ANGLE_TRY(scope.PopScope(context, __FILE__, ANGLE_FUNCTION, __LINE__))

#define ANGLE_WGPU_SCOPED_DEBUG_TRY(context, command)                                            \
    do                                                                                           \
    {                                                                                            \
        ::rx::webgpu::DebugErrorScope _errorScope = ANGLE_WGPU_BEGIN_DEBUG_ERROR_SCOPE(context); \
        (command);                                                                               \
        ANGLE_WGPU_END_DEBUG_ERROR_SCOPE(context, _errorScope);                                  \
    } while (0)

#define ANGLE_GL_OBJECTS_X(PROC) \
    PROC(Buffer)                 \
    PROC(Context)                \
    PROC(Framebuffer)            \
    PROC(Query)                  \
    PROC(Program)                \
    PROC(ProgramExecutable)      \
    PROC(Sampler)                \
    PROC(Texture)                \
    PROC(TransformFeedback)      \
    PROC(VertexArray)

#define ANGLE_EGL_OBJECTS_X(PROC) \
    PROC(Display)                 \
    PROC(Image)                   \
    PROC(Surface)                 \
    PROC(Sync)

#define ANGLE_WGPU_OBJECTS_X(PROC) \
    PROC(Adapter)                  \
    PROC(BindGroup)                \
    PROC(BindGroupLayout)          \
    PROC(Buffer)                   \
    PROC(CommandBuffer)            \
    PROC(CommandEncoder)           \
    PROC(ComputePassEncoder)       \
    PROC(ComputePipeline)          \
    PROC(Device)                   \
    PROC(ExternalTexture)          \
    PROC(Instance)                 \
    PROC(PipelineLayout)           \
    PROC(QuerySet)                 \
    PROC(Queue)                    \
    PROC(RenderBundle)             \
    PROC(RenderBundleEncoder)      \
    PROC(RenderPassEncoder)        \
    PROC(RenderPipeline)           \
    PROC(Sampler)                  \
    PROC(ShaderModule)             \
    PROC(SharedBufferMemory)       \
    PROC(SharedFence)              \
    PROC(SharedTextureMemory)      \
    PROC(Surface)                  \
    PROC(Texture)                  \
    PROC(TextureView)

namespace rx
{

class ContextWgpu;
class DisplayWgpu;

#define ANGLE_PRE_DECLARE_WGPU_OBJECT(OBJ) class OBJ##Wgpu;

ANGLE_GL_OBJECTS_X(ANGLE_PRE_DECLARE_WGPU_OBJECT)
ANGLE_EGL_OBJECTS_X(ANGLE_PRE_DECLARE_WGPU_OBJECT)

namespace webgpu
{

#define ANGLE_DECLARE_WGPU_HANDLE_REF_FUNCS(OBJ)     \
    inline void AddRefWGPUCHandle(WGPU##OBJ handle)  \
    {                                                \
        if (handle)                                  \
        {                                            \
            wgpu##OBJ##AddRef(handle);               \
        }                                            \
    }                                                \
                                                     \
    inline void ReleaseWGPUCHandle(WGPU##OBJ handle) \
    {                                                \
        if (handle)                                  \
        {                                            \
            wgpu##OBJ##Release(handle);              \
        }                                            \
    }

ANGLE_WGPU_OBJECTS_X(ANGLE_DECLARE_WGPU_HANDLE_REF_FUNCS)
#undef ANGLE_DECLARE_WGPU_HANDLE_REF_FUNCS

template <typename CType>
class WrapperBase
{
  public:
    using ObjectType = CType;

    WrapperBase() = default;
    WrapperBase(const WrapperBase<CType> &other) : mHandle(other.mHandle)
    {
        AddRefWGPUCHandle(mHandle);
    }

    ~WrapperBase() { ReleaseWGPUCHandle(mHandle); }

    WrapperBase<CType> &operator=(const WrapperBase<CType> &other)
    {
        if (&other != this)
        {
            ReleaseWGPUCHandle(mHandle);
            mHandle = other.mHandle;
            AddRefWGPUCHandle(mHandle);
        }
        return *this;
    }

    WrapperBase(WrapperBase<CType> &&other)
    {
        mHandle       = other.mHandle;
        other.mHandle = nullptr;
    }

    WrapperBase &operator=(WrapperBase<CType> &&other)
    {
        if (&other != this)
        {
            ReleaseWGPUCHandle(mHandle);
            mHandle       = other.mHandle;
            other.mHandle = nullptr;
        }
        return *this;
    }

    WrapperBase(std::nullptr_t) {}

    WrapperBase &operator=(std::nullptr_t)
    {
        ReleaseWGPUCHandle(mHandle);
        mHandle = nullptr;
        return *this;
    }

    bool operator==(const WrapperBase<CType> &other) const { return mHandle == other.mHandle; }

    bool operator!=(const WrapperBase<CType> &other) const { return !(*this == other); }

    bool operator==(std::nullptr_t) const { return mHandle == nullptr; }

    bool operator!=(std::nullptr_t) const { return mHandle != nullptr; }

    explicit operator bool() const { return mHandle != nullptr; }

    const CType &get() const { return mHandle; }

    static WrapperBase<CType> Acquire(CType handle)
    {
        WrapperBase<CType> result;
        result.mHandle = handle;
        return result;
    }

    size_t hash() const
    {
        std::hash<CType> hasher;
        return hasher(mHandle);
    }

  private:
    CType mHandle = nullptr;
};

#define ANGLE_DECLARE_WGPU_OBJECT_WRAPPER(OBJ) using OBJ##Handle = WrapperBase<WGPU##OBJ>;

ANGLE_WGPU_OBJECTS_X(ANGLE_DECLARE_WGPU_OBJECT_WRAPPER)
#undef ANGLE_DECLARE_WGPU_OBJECT_WRAPPER

template <typename T>
struct ImplTypeHelper;

#define ANGLE_IMPL_TYPE_HELPER(frontendNamespace, OBJ) \
    template <>                                        \
    struct ImplTypeHelper<frontendNamespace::OBJ>      \
    {                                                  \
        using ImplType = rx::OBJ##Wgpu;                \
    };
#define ANGLE_IMPL_TYPE_HELPER_GL(OBJ) ANGLE_IMPL_TYPE_HELPER(gl, OBJ)
#define ANGLE_IMPL_TYPE_HELPER_EGL(OBJ) ANGLE_IMPL_TYPE_HELPER(egl, OBJ)

ANGLE_GL_OBJECTS_X(ANGLE_IMPL_TYPE_HELPER_GL)
ANGLE_EGL_OBJECTS_X(ANGLE_IMPL_TYPE_HELPER_EGL)

#undef ANGLE_IMPL_TYPE_HELPER_GL
#undef ANGLE_IMPL_TYPE_HELPER_EGL

template <typename T>
using GetImplType = typename ImplTypeHelper<T>::ImplType;

template <typename T>
GetImplType<T> *GetImpl(const T *glObject)
{
    return GetImplAs<GetImplType<T>>(glObject);
}

constexpr size_t kUnpackedDepthIndex   = gl::IMPLEMENTATION_MAX_DRAW_BUFFERS;
constexpr size_t kUnpackedStencilIndex = gl::IMPLEMENTATION_MAX_DRAW_BUFFERS + 1;
constexpr uint32_t kUnpackedColorBuffersMask =
    angle::BitMask<uint32_t>(gl::IMPLEMENTATION_MAX_DRAW_BUFFERS);
// WebGPU image level index.
using LevelIndex = gl::LevelIndexWrapper<uint32_t>;

class ErrorScope : public angle::NonCopyable
{
  public:
    ErrorScope(webgpu::InstanceHandle instance,
               webgpu::DeviceHandle device,
               WGPUErrorFilter errorType);
    ~ErrorScope();

    angle::Result PopScope(ContextWgpu *context,
                           const char *file,
                           const char *function,
                           unsigned int line);

  private:
    webgpu::InstanceHandle mInstance;
    webgpu::DeviceHandle mDevice;
    bool mActive = false;
};

class NoOpErrorScope : public angle::NonCopyable
{
  public:
    NoOpErrorScope(webgpu::InstanceHandle instance,
                   webgpu::DeviceHandle device,
                   WGPUErrorFilter errorType)
    {}
    ~NoOpErrorScope() {}

    angle::Result PopScope(ContextWgpu *context,
                           const char *file,
                           const char *function,
                           unsigned int line)
    {
        return angle::Result::Continue;
    }
};

#if defined(ANGLE_ENABLE_ASSERTS)
using DebugErrorScope = ErrorScope;
#else
using DebugErrorScope = NoOpErrorScope;
#endif

enum class RenderPassClosureReason
{
    NewRenderPass,
    FramebufferBindingChange,
    FramebufferInternalChange,
    GLFlush,
    GLFinish,
    EGLSwapBuffers,
    GLReadPixels,
    IndexRangeReadback,
    VertexArrayStreaming,
    VertexArrayLineLoop,
    CopyBufferToTexture,

    InvalidEnum,
    EnumCount = InvalidEnum,
};

struct ClearValues
{
    gl::ColorF clearColor;
    uint32_t depthSlice;
    float depthValue;
    uint32_t stencilValue;
};

class ClearValuesArray final
{
  public:
    ClearValuesArray();
    ~ClearValuesArray();

    ClearValuesArray(const ClearValuesArray &other);
    ClearValuesArray &operator=(const ClearValuesArray &rhs);

    void store(uint32_t index, const ClearValues &clearValues);

    gl::DrawBufferMask getColorMask() const;
    void reset()
    {
        mValues.fill({});
        mEnabled.reset();
    }
    void reset(size_t index)
    {
        mValues[index] = {};
        mEnabled.reset(index);
    }
    void resetDepth()
    {
        mValues[kUnpackedDepthIndex] = {};
        mEnabled.reset(kUnpackedDepthIndex);
    }
    void resetStencil()
    {
        mValues[kUnpackedStencilIndex] = {};
        mEnabled.reset(kUnpackedStencilIndex);
    }
    const ClearValues &operator[](size_t index) const { return mValues[index]; }

    bool empty() const { return mEnabled.none(); }
    bool any() const { return mEnabled.any(); }

    bool test(size_t index) const { return mEnabled.test(index); }

    float getDepthValue() const { return mValues[kUnpackedDepthIndex].depthValue; }
    uint32_t getStencilValue() const { return mValues[kUnpackedStencilIndex].stencilValue; }
    bool hasDepth() const { return mEnabled.test(kUnpackedDepthIndex); }
    bool hasStencil() const { return mEnabled.test(kUnpackedStencilIndex); }

  private:
    gl::AttachmentArray<ClearValues> mValues;
    gl::AttachmentsMask mEnabled;
};

struct PackedRenderPassColorAttachment
{
    TextureViewHandle view;
    uint32_t depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    WGPULoadOp loadOp   = WGPULoadOp_Undefined;
    WGPUStoreOp storeOp = WGPUStoreOp_Undefined;
    gl::ColorF clearValue;
};

bool operator==(const PackedRenderPassColorAttachment &a, const PackedRenderPassColorAttachment &b);

struct PackedRenderPassDepthStencilAttachment
{
    TextureViewHandle view;
    WGPULoadOp depthLoadOp     = WGPULoadOp_Undefined;
    WGPUStoreOp depthStoreOp   = WGPUStoreOp_Undefined;
    bool depthReadOnly         = false;
    float depthClearValue      = NAN;
    WGPULoadOp stencilLoadOp   = WGPULoadOp_Undefined;
    WGPUStoreOp stencilStoreOp = WGPUStoreOp_Undefined;
    bool stencilReadOnly       = false;
    uint32_t stencilClearValue = 0;
};

bool operator==(const PackedRenderPassDepthStencilAttachment &a,
                const PackedRenderPassDepthStencilAttachment &b);

struct PackedRenderPassDescriptor
{
    angle::FixedVector<PackedRenderPassColorAttachment, gl::IMPLEMENTATION_MAX_DRAW_BUFFERS>
        colorAttachments;
    std::optional<PackedRenderPassDepthStencilAttachment> depthStencilAttachment;
};

bool operator==(const PackedRenderPassDescriptor &a, const PackedRenderPassDescriptor &b);
bool operator!=(const PackedRenderPassDescriptor &a, const PackedRenderPassDescriptor &b);

RenderPassEncoderHandle CreateRenderPass(webgpu::CommandEncoderHandle commandEncoder,
                                         const webgpu::PackedRenderPassDescriptor &desc);

void GenerateCaps(const WGPULimits &limitWgpu,
                  gl::Caps *glCaps,
                  gl::TextureCapsMap *glTextureCapsMap,
                  gl::Extensions *glExtensions,
                  gl::Limitations *glLimitations,
                  egl::Caps *eglCaps,
                  egl::DisplayExtensions *eglExtensions,
                  gl::Version *maxSupportedESVersion);

DisplayWgpu *GetDisplay(const gl::Context *context);
webgpu::DeviceHandle GetDevice(const gl::Context *context);
webgpu::InstanceHandle GetInstance(const gl::Context *context);
PackedRenderPassColorAttachment CreateNewClearColorAttachment(const gl::ColorF &clearValue,
                                                              uint32_t depthSlice,
                                                              TextureViewHandle textureView);
PackedRenderPassDepthStencilAttachment CreateNewDepthStencilAttachment(
    float depthClearValue,
    uint32_t stencilClearValue,
    TextureViewHandle textureView,
    bool hasDepthValue   = false,
    bool hasStencilValue = false);

bool IsWgpuError(WGPUWaitStatus waitStatus);
bool IsWgpuError(WGPUMapAsyncStatus mapAsyncStatus);

bool IsStripPrimitiveTopology(WGPUPrimitiveTopology topology);

// Required alignments for buffer sizes and mapping
constexpr size_t kBufferSizeAlignment         = 4;
constexpr size_t kBufferCopyToBufferAlignment = 4;
constexpr size_t kBufferMapSizeAlignment   = kBufferSizeAlignment;
constexpr size_t kBufferMapOffsetAlignment = 8;

// Required alignments for texture row uploads
constexpr size_t kTextureRowSizeAlignment = 256;

// Structs in WGPU's uniform address space are always aligned to 16. I.e. RequiredAlignOf(struct S,
// uniform) = roundUp(16, AlignOf(S)) and AlignOf(S) is at most 16.
constexpr size_t kUniformStructAlignment = 16;

// min and max LOD clamp values.
constexpr float kWGPUMinLod = 0.0;
constexpr float kWGPUMaxLod = 32.0;

}  // namespace webgpu

namespace wgpu_gl
{
gl::LevelIndex GetLevelIndex(webgpu::LevelIndex levelWgpu, gl::LevelIndex baseLevel);
gl::Extents GetExtents(WGPUExtent3D wgpuExtent);
}  // namespace wgpu_gl

namespace gl_wgpu
{
webgpu::LevelIndex getLevelIndex(gl::LevelIndex levelGl, gl::LevelIndex baseLevel);
WGPUTextureViewDimension GetWgpuTextureViewDimension(gl::TextureType textureType);
WGPUTextureDimension GetWgpuTextureDimension(gl::TextureType glTextureType);
WGPUExtent3D GetExtent3D(const gl::Extents &glExtent);

WGPUPrimitiveTopology GetPrimitiveTopology(gl::PrimitiveMode mode);

WGPUIndexFormat GetIndexFormat(gl::DrawElementsType drawElementsTYpe);
WGPUFrontFace GetFrontFace(GLenum frontFace);
WGPUCullMode GetCullMode(gl::CullFaceMode mode, bool cullFaceEnabled);
WGPUColorWriteMask GetColorWriteMask(bool r, bool g, bool b, bool a);

WGPUBlendFactor GetBlendFactor(gl::BlendFactorType blendFactor);
WGPUBlendOperation GetBlendEquation(gl::BlendEquationType blendEquation);

WGPUCompareFunction GetCompareFunc(const GLenum glCompareFunc, bool testEnabled);
WGPUTextureSampleType GetTextureSampleType(gl::SamplerFormat samplerFormat);
WGPUStencilOperation GetStencilOp(const GLenum glStencilOp);
WGPUFilterMode GetFilter(const GLenum filter);
WGPUMipmapFilterMode GetSamplerMipmapMode(const GLenum filter);
WGPUAddressMode GetSamplerAddressMode(const GLenum wrap);

WGPUSamplerDescriptor GetWgpuSamplerDesc(const gl::SamplerState *samplerState);

uint32_t GetFirstIndexForDrawCall(gl::DrawElementsType indexType, const void *indices);
}  // namespace gl_wgpu

// Number of reserved binding slots to implement the default uniform block
constexpr uint32_t kReservedPerStageDefaultUniformSlotCount = 0;

}  // namespace rx

namespace std
{
template <typename CType>
struct hash<rx::webgpu::WrapperBase<CType>>
{
    size_t operator()(const rx::webgpu::WrapperBase<CType> &obj) const { return obj.hash(); }
};
}  // namespace std

#endif  // LIBANGLE_RENDERER_WGPU_WGPU_UTILS_H_
