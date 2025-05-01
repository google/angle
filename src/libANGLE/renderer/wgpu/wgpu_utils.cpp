//
// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libANGLE/renderer/wgpu/wgpu_utils.h"

#include "common/log_utils.h"
#include "libANGLE/renderer/renderer_utils.h"
#include "libANGLE/renderer/wgpu/ContextWgpu.h"
#include "libANGLE/renderer/wgpu/DisplayWgpu.h"
#include "libANGLE/renderer/wgpu/wgpu_pipeline_state.h"

namespace rx
{

namespace webgpu
{
DisplayWgpu *GetDisplay(const gl::Context *context)
{
    ContextWgpu *contextWgpu = GetImpl(context);
    return contextWgpu->getDisplay();
}

wgpu::Device GetDevice(const gl::Context *context)
{
    DisplayWgpu *display = GetDisplay(context);
    return display->getDevice();
}

wgpu::Instance GetInstance(const gl::Context *context)
{
    DisplayWgpu *display = GetDisplay(context);
    return display->getInstance();
}

WGPURenderPassColorAttachment CreateNewClearColorAttachment(WGPUColor clearValue,
                                                            uint32_t depthSlice,
                                                            TextureViewHandle textureView)
{
    WGPURenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_INIT;
    colorAttachment.view                          = textureView.get();
    colorAttachment.depthSlice = depthSlice;
    colorAttachment.loadOp                        = WGPULoadOp_Clear;
    colorAttachment.storeOp                       = WGPUStoreOp_Store;
    colorAttachment.clearValue = clearValue;

    return colorAttachment;
}

WGPURenderPassDepthStencilAttachment CreateNewDepthStencilAttachment(float depthClearValue,
                                                                     uint32_t stencilClearValue,
                                                                     TextureViewHandle textureView,
                                                                     bool hasDepthValue,
                                                                     bool hasStencilValue)
{
    WGPURenderPassDepthStencilAttachment depthStencilAttachment =
        WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_INIT;
    depthStencilAttachment.view = textureView.get();
    // WebGPU requires that depth/stencil attachments have a load op if the correlated ReadOnly
    // value is set to false, so we make sure to set the value here to to support cases where only a
    // depth or stencil mask is set.
    depthStencilAttachment.depthReadOnly   = !hasDepthValue;
    depthStencilAttachment.stencilReadOnly = !hasStencilValue;
    if (hasDepthValue)
    {
        depthStencilAttachment.depthLoadOp     = WGPULoadOp_Clear;
        depthStencilAttachment.depthStoreOp    = WGPUStoreOp_Store;
        depthStencilAttachment.depthClearValue = depthClearValue;
    }
    if (hasStencilValue)
    {
        depthStencilAttachment.stencilLoadOp     = WGPULoadOp_Clear;
        depthStencilAttachment.stencilStoreOp    = WGPUStoreOp_Store;
        depthStencilAttachment.stencilClearValue = stencilClearValue;
    }

    return depthStencilAttachment;
}

bool IsWgpuError(wgpu::WaitStatus waitStatus)
{
    return waitStatus != wgpu::WaitStatus::Success;
}

bool IsWgpuError(wgpu::MapAsyncStatus mapAsyncStatus)
{
    return mapAsyncStatus != wgpu::MapAsyncStatus::Success;
}

ClearValuesArray::ClearValuesArray() : mValues{}, mEnabled{} {}
ClearValuesArray::~ClearValuesArray() = default;

ClearValuesArray::ClearValuesArray(const ClearValuesArray &other)          = default;
ClearValuesArray &ClearValuesArray::operator=(const ClearValuesArray &rhs) = default;

void ClearValuesArray::store(uint32_t index, const ClearValues &clearValues)
{
    mValues[index] = clearValues;
    mEnabled.set(index);
}

gl::DrawBufferMask ClearValuesArray::getColorMask() const
{
    return gl::DrawBufferMask(mEnabled.bits() & kUnpackedColorBuffersMask);
}

void GenerateCaps(const wgpu::Limits &limitsWgpu,
                  gl::Caps *glCaps,
                  gl::TextureCapsMap *glTextureCapsMap,
                  gl::Extensions *glExtensions,
                  gl::Limitations *glLimitations,
                  egl::Caps *eglCaps,
                  egl::DisplayExtensions *eglExtensions,
                  gl::Version *maxSupportedESVersion)
{
    // WebGPU does not support separate front/back stencil masks.
    glLimitations->noSeparateStencilRefsAndMasks = true;

    // OpenGL ES extensions
    glExtensions->debugMarkerEXT              = true;
    glExtensions->textureUsageANGLE           = true;
    glExtensions->translatedShaderSourceANGLE = true;
    glExtensions->vertexArrayObjectOES        = true;
    glExtensions->elementIndexUintOES         = true;

    glExtensions->textureStorageEXT = true;
    glExtensions->rgb8Rgba8OES      = true;

    // OpenGL ES caps
    glCaps->maxElementIndex       = std::numeric_limits<GLuint>::max() - 1;
    glCaps->max3DTextureSize      = rx::LimitToInt(limitsWgpu.maxTextureDimension3D);
    glCaps->max2DTextureSize      = rx::LimitToInt(limitsWgpu.maxTextureDimension2D);
    glCaps->maxArrayTextureLayers = rx::LimitToInt(limitsWgpu.maxTextureArrayLayers);
    glCaps->maxLODBias            = 0.0f;
    glCaps->maxCubeMapTextureSize = rx::LimitToInt(limitsWgpu.maxTextureDimension2D);
    glCaps->maxRenderbufferSize   = rx::LimitToInt(limitsWgpu.maxTextureDimension2D);
    glCaps->minAliasedPointSize   = 1.0f;
    glCaps->maxAliasedPointSize   = 1.0f;
    glCaps->minAliasedLineWidth   = 1.0f;
    glCaps->maxAliasedLineWidth   = 1.0f;

    // "descriptor.sampleCount must be either 1 or 4."
    constexpr uint32_t kMaxSampleCount = 4;

    glCaps->maxDrawBuffers         = rx::LimitToInt(limitsWgpu.maxColorAttachments);
    glCaps->maxFramebufferWidth    = rx::LimitToInt(limitsWgpu.maxTextureDimension2D);
    glCaps->maxFramebufferHeight   = rx::LimitToInt(limitsWgpu.maxTextureDimension2D);
    glCaps->maxFramebufferSamples  = kMaxSampleCount;
    glCaps->maxColorAttachments    = rx::LimitToInt(limitsWgpu.maxColorAttachments);
    glCaps->maxViewportWidth       = rx::LimitToInt(limitsWgpu.maxTextureDimension2D);
    glCaps->maxViewportHeight      = glCaps->maxViewportWidth;
    glCaps->maxSampleMaskWords     = 1;
    glCaps->maxColorTextureSamples = kMaxSampleCount;
    glCaps->maxDepthTextureSamples = kMaxSampleCount;
    glCaps->maxIntegerSamples      = kMaxSampleCount;
    glCaps->maxServerWaitTimeout   = 0;

    glCaps->maxVertexAttribRelativeOffset = (1u << kAttributeOffsetMaxBits) - 1;
    glCaps->maxVertexAttribBindings =
        rx::LimitToInt(std::min(limitsWgpu.maxVertexBuffers, limitsWgpu.maxVertexAttributes));
    glCaps->maxVertexAttribStride =
        rx::LimitToInt(std::min(limitsWgpu.maxVertexBufferArrayStride,
                                static_cast<uint32_t>(std::numeric_limits<uint16_t>::max())));
    glCaps->maxElementsIndices  = std::numeric_limits<GLint>::max();
    glCaps->maxElementsVertices = std::numeric_limits<GLint>::max();
    glCaps->vertexHighpFloat.setIEEEFloat();
    glCaps->vertexMediumpFloat.setIEEEHalfFloat();
    glCaps->vertexLowpFloat.setIEEEHalfFloat();
    glCaps->fragmentHighpFloat.setIEEEFloat();
    glCaps->fragmentMediumpFloat.setIEEEHalfFloat();
    glCaps->fragmentLowpFloat.setIEEEHalfFloat();
    glCaps->vertexHighpInt.setTwosComplementInt(32);
    glCaps->vertexMediumpInt.setTwosComplementInt(16);
    glCaps->vertexLowpInt.setTwosComplementInt(16);
    glCaps->fragmentHighpInt.setTwosComplementInt(32);
    glCaps->fragmentMediumpInt.setTwosComplementInt(16);
    glCaps->fragmentLowpInt.setTwosComplementInt(16);

    // Clamp the maxUniformBlockSize to 64KB (majority of devices support up to this size
    // currently), on AMD the maxUniformBufferRange is near uint32_t max.
    GLuint maxUniformBlockSize = static_cast<GLuint>(
        std::min(static_cast<uint64_t>(0x10000), limitsWgpu.maxUniformBufferBindingSize));

    const GLuint maxUniformVectors    = maxUniformBlockSize / (sizeof(GLfloat) * 4);
    const GLuint maxUniformComponents = maxUniformVectors * 4;

    const int32_t maxPerStageUniformBuffers = rx::LimitToInt(
        limitsWgpu.maxUniformBuffersPerShaderStage - kReservedPerStageDefaultUniformSlotCount);

    // There is no additional limit to the combined number of components.  We can have up to a
    // maximum number of uniform buffers, each having the maximum number of components.  Note that
    // this limit includes both components in and out of uniform buffers.
    //
    // This value is limited to INT_MAX to avoid overflow when queried from glGetIntegerv().
    const uint64_t maxCombinedUniformComponents =
        std::min<uint64_t>(static_cast<uint64_t>(maxPerStageUniformBuffers +
                                                 kReservedPerStageDefaultUniformSlotCount) *
                               maxUniformComponents,
                           std::numeric_limits<GLint>::max());

    for (gl::ShaderType shaderType : gl::AllShaderTypes())
    {
        glCaps->maxShaderUniformBlocks[shaderType] = maxPerStageUniformBuffers;
        glCaps->maxShaderTextureImageUnits[shaderType] =
            rx::LimitToInt(limitsWgpu.maxSamplersPerShaderStage);
        glCaps->maxShaderStorageBlocks[shaderType]             = 0;
        glCaps->maxShaderUniformComponents[shaderType]         = 0;
        glCaps->maxShaderAtomicCounterBuffers[shaderType]      = 0;
        glCaps->maxShaderAtomicCounters[shaderType]            = 0;
        glCaps->maxShaderImageUniforms[shaderType]             = 0;
        glCaps->maxCombinedShaderUniformComponents[shaderType] = maxCombinedUniformComponents;
    }

    const GLint maxVaryingComponents = rx::LimitToInt(limitsWgpu.maxInterStageShaderVariables * 4);

    glCaps->maxVertexAttributes = rx::LimitToInt(
        limitsWgpu.maxVertexBuffers);  // WebGPU has maxVertexBuffers and maxVertexAttributes but
                                       // since each vertex attribute can use a unique buffer, we
                                       // are limited by the total number of vertex buffers
    glCaps->maxVertexUniformVectors =
        maxUniformVectors;  // Uniforms are implemented using a uniform buffer, so the max number of
                            // uniforms we can support is the max buffer range divided by the size
                            // of a single uniform (4X float).
    glCaps->maxVertexOutputComponents = maxVaryingComponents;

    glCaps->maxFragmentUniformVectors     = maxUniformVectors;
    glCaps->maxFragmentInputComponents    = maxVaryingComponents;
    glCaps->minProgramTextureGatherOffset = 0;
    glCaps->maxProgramTextureGatherOffset = 0;
    glCaps->minProgramTexelOffset         = -8;
    glCaps->maxProgramTexelOffset         = 7;

    glCaps->maxComputeWorkGroupCount       = {0, 0, 0};
    glCaps->maxComputeWorkGroupSize        = {0, 0, 0};
    glCaps->maxComputeWorkGroupInvocations = 0;
    glCaps->maxComputeSharedMemorySize     = 0;

    // Only 2 stages (vertex+fragment) are supported.
    constexpr uint32_t kShaderStageCount = 2;

    glCaps->maxUniformBufferBindings = maxPerStageUniformBuffers * kShaderStageCount;
    glCaps->maxUniformBlockSize      = rx::LimitToInt(limitsWgpu.maxBufferSize);
    glCaps->uniformBufferOffsetAlignment =
        rx::LimitToInt(limitsWgpu.minUniformBufferOffsetAlignment);
    glCaps->maxCombinedUniformBlocks = glCaps->maxUniformBufferBindings;
    glCaps->maxVaryingComponents     = maxVaryingComponents;
    glCaps->maxVaryingVectors        = rx::LimitToInt(limitsWgpu.maxInterStageShaderVariables);
    glCaps->maxCombinedTextureImageUnits =
        rx::LimitToInt(limitsWgpu.maxSamplersPerShaderStage * kShaderStageCount);
    glCaps->maxCombinedShaderOutputResources = 0;

    glCaps->maxUniformLocations                = maxUniformVectors;
    glCaps->maxAtomicCounterBufferBindings     = 0;
    glCaps->maxAtomicCounterBufferSize         = 0;
    glCaps->maxCombinedAtomicCounterBuffers    = 0;
    glCaps->maxCombinedAtomicCounters          = 0;
    glCaps->maxImageUnits                      = 0;
    glCaps->maxCombinedImageUniforms           = 0;
    glCaps->maxShaderStorageBufferBindings     = 0;
    glCaps->maxShaderStorageBlockSize          = 0;
    glCaps->maxCombinedShaderStorageBlocks     = 0;
    glCaps->shaderStorageBufferOffsetAlignment = 0;

    glCaps->maxTransformFeedbackInterleavedComponents = 0;
    glCaps->maxTransformFeedbackSeparateAttributes    = 0;
    glCaps->maxTransformFeedbackSeparateComponents    = 0;

    glCaps->lineWidthGranularity    = 0.0f;
    glCaps->minMultisampleLineWidth = 0.0f;
    glCaps->maxMultisampleLineWidth = 0.0f;

    glCaps->maxTextureBufferSize         = 0;
    glCaps->textureBufferOffsetAlignment = 0;

    glCaps->maxSamples = kMaxSampleCount;

    // Max version
    *maxSupportedESVersion = gl::Version(3, 2);

    // OpenGL ES texture caps
    InitMinimumTextureCapsMap(*maxSupportedESVersion, *glExtensions, glTextureCapsMap);

    // EGL caps
    eglCaps->textureNPOT = true;

    // EGL extensions
    eglExtensions->createContextRobustness            = true;
    eglExtensions->postSubBuffer                      = true;
    eglExtensions->createContext                      = true;
    eglExtensions->image                              = true;
    eglExtensions->imageBase                          = true;
    eglExtensions->glTexture2DImage                   = true;
    eglExtensions->glTextureCubemapImage              = true;
    eglExtensions->glTexture3DImage                   = true;
    eglExtensions->glRenderbufferImage                = true;
    eglExtensions->getAllProcAddresses                = true;
    eglExtensions->noConfigContext                    = true;
    eglExtensions->directComposition                  = true;
    eglExtensions->createContextNoError               = true;
    eglExtensions->createContextWebGLCompatibility    = true;
    eglExtensions->createContextBindGeneratesResource = true;
    eglExtensions->swapBuffersWithDamage              = true;
    eglExtensions->pixelFormatFloat                   = true;
    eglExtensions->surfacelessContext                 = true;
    eglExtensions->displayTextureShareGroup           = true;
    eglExtensions->displaySemaphoreShareGroup         = true;
    eglExtensions->createContextClientArrays          = true;
    eglExtensions->programCacheControlANGLE           = true;
    eglExtensions->robustResourceInitializationANGLE  = true;
}

bool IsStripPrimitiveTopology(WGPUPrimitiveTopology topology)
{
    switch (topology)
    {
        case WGPUPrimitiveTopology_LineStrip:
        case WGPUPrimitiveTopology_TriangleStrip:
            return true;

        default:
            return false;
    }
}

ErrorScope::ErrorScope(wgpu::Instance instance, wgpu::Device device, wgpu::ErrorFilter errorType)
    : mInstance(instance), mDevice(device)
{
    mDevice.PushErrorScope(errorType);
    mActive = true;
}

ErrorScope::~ErrorScope()
{
    ANGLE_UNUSED_VARIABLE(PopScope(nullptr, nullptr, nullptr, 0));
}

angle::Result ErrorScope::PopScope(ContextWgpu *context,
                                   const char *file,
                                   const char *function,
                                   unsigned int line)
{
    if (!mActive)
    {
        return angle::Result::Continue;
    }
    mActive = false;

    bool hadError  = false;
    wgpu::Future f = mDevice.PopErrorScope(
        wgpu::CallbackMode::WaitAnyOnly,
        [context, file, function, line, &hadError](wgpu::PopErrorScopeStatus status,
                                                   wgpu::ErrorType type, char const *message) {
            if (type == wgpu::ErrorType::NoError)
            {
                return;
            }

            if (context)
            {
                ASSERT(file);
                ASSERT(function);
                context->handleError(GL_INVALID_OPERATION, message, file, function, line);
            }
            else
            {
                ERR() << "Unhandled WebGPU error: " << message;
            }
            hadError = true;
        });
    mInstance.WaitAny(f, -1);

    return hadError ? angle::Result::Stop : angle::Result::Continue;
}

}  // namespace webgpu

namespace wgpu_gl
{
gl::LevelIndex GetLevelIndex(webgpu::LevelIndex levelWgpu, gl::LevelIndex baseLevel)
{
    return gl::LevelIndex(levelWgpu.get() + baseLevel.get());
}

gl::Extents GetExtents(WGPUExtent3D wgpuExtent)
{
    gl::Extents glExtent;
    glExtent.width  = wgpuExtent.width;
    glExtent.height = wgpuExtent.height;
    glExtent.depth  = wgpuExtent.depthOrArrayLayers;
    return glExtent;
}
}  // namespace wgpu_gl

namespace gl_wgpu
{
webgpu::LevelIndex getLevelIndex(gl::LevelIndex levelGl, gl::LevelIndex baseLevel)
{
    ASSERT(baseLevel <= levelGl);
    return webgpu::LevelIndex(levelGl.get() - baseLevel.get());
}

WGPUExtent3D GetExtent3D(const gl::Extents &glExtent)
{
    WGPUExtent3D wgpuExtent       = WGPU_EXTENT_3D_INIT;
    wgpuExtent.width              = glExtent.width;
    wgpuExtent.height             = glExtent.height;
    wgpuExtent.depthOrArrayLayers = glExtent.depth;
    return wgpuExtent;
}

WGPUPrimitiveTopology GetPrimitiveTopology(gl::PrimitiveMode mode)
{
    switch (mode)
    {
        case gl::PrimitiveMode::Points:
            return WGPUPrimitiveTopology_PointList;
        case gl::PrimitiveMode::Lines:
            return WGPUPrimitiveTopology_LineList;
        case gl::PrimitiveMode::LineLoop:
            return WGPUPrimitiveTopology_LineStrip;  // Emulated
        case gl::PrimitiveMode::LineStrip:
            return WGPUPrimitiveTopology_LineStrip;
        case gl::PrimitiveMode::Triangles:
            return WGPUPrimitiveTopology_TriangleList;
        case gl::PrimitiveMode::TriangleStrip:
            return WGPUPrimitiveTopology_TriangleStrip;
        case gl::PrimitiveMode::TriangleFan:
            UNIMPLEMENTED();
            return WGPUPrimitiveTopology_TriangleList;  // Emulated
        default:
            UNREACHABLE();
            return WGPUPrimitiveTopology_Undefined;
    }
}

WGPUIndexFormat GetIndexFormat(gl::DrawElementsType drawElementsType)
{
    switch (drawElementsType)
    {
        case gl::DrawElementsType::UnsignedByte:
            return WGPUIndexFormat_Uint16;  // Emulated
        case gl::DrawElementsType::UnsignedShort:
            return WGPUIndexFormat_Uint16;
        case gl::DrawElementsType::UnsignedInt:
            return WGPUIndexFormat_Uint32;

        default:
            UNREACHABLE();
            return WGPUIndexFormat_Undefined;
    }
}

WGPUFrontFace GetFrontFace(GLenum frontFace)
{
    switch (frontFace)
    {
        case GL_CW:
            return WGPUFrontFace_CW;
        case GL_CCW:
            return WGPUFrontFace_CCW;

        default:
            UNREACHABLE();
            return WGPUFrontFace_Undefined;
    }
}

WGPUCullMode GetCullMode(gl::CullFaceMode mode, bool cullFaceEnabled)
{
    if (!cullFaceEnabled)
    {
        return WGPUCullMode_None;
    }

    switch (mode)
    {
        case gl::CullFaceMode::Front:
            return WGPUCullMode_Front;
        case gl::CullFaceMode::Back:
            return WGPUCullMode_Back;
        case gl::CullFaceMode::FrontAndBack:
            UNIMPLEMENTED();
            return WGPUCullMode_None;  // Emulated
        default:
            UNREACHABLE();
            return WGPUCullMode_None;
    }
}

WGPUColorWriteMask GetColorWriteMask(bool r, bool g, bool b, bool a)
{
    return (r ? WGPUColorWriteMask_Red : WGPUColorWriteMask_None) |
           (g ? WGPUColorWriteMask_Green : WGPUColorWriteMask_None) |
           (b ? WGPUColorWriteMask_Blue : WGPUColorWriteMask_None) |
           (a ? WGPUColorWriteMask_Alpha : WGPUColorWriteMask_None);
}

WGPUBlendFactor GetBlendFactor(gl::BlendFactorType blendFactor)
{
    switch (blendFactor)
    {
        case gl::BlendFactorType::Zero:
            return WGPUBlendFactor_Zero;

        case gl::BlendFactorType::One:
            return WGPUBlendFactor_One;

        case gl::BlendFactorType::SrcColor:
            return WGPUBlendFactor_Src;

        case gl::BlendFactorType::OneMinusSrcColor:
            return WGPUBlendFactor_OneMinusSrc;

        case gl::BlendFactorType::SrcAlpha:
            return WGPUBlendFactor_SrcAlpha;

        case gl::BlendFactorType::OneMinusSrcAlpha:
            return WGPUBlendFactor_OneMinusSrcAlpha;

        case gl::BlendFactorType::DstAlpha:
            return WGPUBlendFactor_DstAlpha;

        case gl::BlendFactorType::OneMinusDstAlpha:
            return WGPUBlendFactor_OneMinusDstAlpha;

        case gl::BlendFactorType::DstColor:
            return WGPUBlendFactor_Dst;

        case gl::BlendFactorType::OneMinusDstColor:
            return WGPUBlendFactor_OneMinusDst;

        case gl::BlendFactorType::SrcAlphaSaturate:
            return WGPUBlendFactor_SrcAlphaSaturated;

        case gl::BlendFactorType::ConstantColor:
            return WGPUBlendFactor_Constant;

        case gl::BlendFactorType::OneMinusConstantColor:
            return WGPUBlendFactor_OneMinusConstant;

        case gl::BlendFactorType::ConstantAlpha:
            UNIMPLEMENTED();
            return WGPUBlendFactor_Undefined;

        case gl::BlendFactorType::OneMinusConstantAlpha:
            UNIMPLEMENTED();
            return WGPUBlendFactor_Undefined;

        case gl::BlendFactorType::Src1Alpha:
            return WGPUBlendFactor_Src1Alpha;

        case gl::BlendFactorType::Src1Color:
            return WGPUBlendFactor_Src1;

        case gl::BlendFactorType::OneMinusSrc1Color:
            return WGPUBlendFactor_OneMinusSrc1;

        case gl::BlendFactorType::OneMinusSrc1Alpha:
            return WGPUBlendFactor_OneMinusSrc1Alpha;

        default:
            UNREACHABLE();
            return WGPUBlendFactor_Undefined;
    }
}

WGPUBlendOperation GetBlendEquation(gl::BlendEquationType blendEquation)
{
    switch (blendEquation)
    {
        case gl::BlendEquationType::Add:
            return WGPUBlendOperation_Add;

        case gl::BlendEquationType::Min:
            return WGPUBlendOperation_Min;

        case gl::BlendEquationType::Max:
            return WGPUBlendOperation_Max;

        case gl::BlendEquationType::Subtract:
            return WGPUBlendOperation_Subtract;

        case gl::BlendEquationType::ReverseSubtract:
            return WGPUBlendOperation_ReverseSubtract;

        case gl::BlendEquationType::Multiply:
        case gl::BlendEquationType::Screen:
        case gl::BlendEquationType::Overlay:
        case gl::BlendEquationType::Darken:
        case gl::BlendEquationType::Lighten:
        case gl::BlendEquationType::Colordodge:
        case gl::BlendEquationType::Colorburn:
        case gl::BlendEquationType::Hardlight:
        case gl::BlendEquationType::Softlight:
        case gl::BlendEquationType::Unused2:
        case gl::BlendEquationType::Difference:
        case gl::BlendEquationType::Unused3:
        case gl::BlendEquationType::Exclusion:
        case gl::BlendEquationType::HslHue:
        case gl::BlendEquationType::HslSaturation:
        case gl::BlendEquationType::HslColor:
        case gl::BlendEquationType::HslLuminosity:
            // EXT_blend_equation_advanced
            UNIMPLEMENTED();
            return WGPUBlendOperation_Undefined;

        default:
            UNREACHABLE();
            return WGPUBlendOperation_Undefined;
    }
}

WGPUTextureViewDimension GetWgpuTextureViewDimension(gl::TextureType textureType)
{
    switch (textureType)
    {
        case gl::TextureType::_2D:
        case gl::TextureType::_2DMultisample:
            return WGPUTextureViewDimension_2D;
        case gl::TextureType::_2DArray:
        case gl::TextureType::_2DMultisampleArray:
            return WGPUTextureViewDimension_2DArray;
        case gl::TextureType::_3D:
            return WGPUTextureViewDimension_3D;
        case gl::TextureType::CubeMap:
            return WGPUTextureViewDimension_Cube;
        case gl::TextureType::CubeMapArray:
            return WGPUTextureViewDimension_CubeArray;
        default:
            UNIMPLEMENTED();
            return WGPUTextureViewDimension_Undefined;
    }
}

WGPUTextureDimension GetWgpuTextureDimension(gl::TextureType glTextureType)
{
    switch (glTextureType)
    {
        // See https://www.w3.org/TR/webgpu/#dom-gputexture-createview.
        case gl::TextureType::_2D:
        case gl::TextureType::_2DArray:
        case gl::TextureType::_2DMultisample:
        case gl::TextureType::_2DMultisampleArray:
        case gl::TextureType::CubeMap:
        case gl::TextureType::CubeMapArray:
        case gl::TextureType::Rectangle:
        case gl::TextureType::External:
        case gl::TextureType::Buffer:
            return WGPUTextureDimension_2D;
        case gl::TextureType::_3D:
        case gl::TextureType::VideoImage:
            return WGPUTextureDimension_3D;
        default:
            UNREACHABLE();
            return WGPUTextureDimension_Undefined;
    }
}

WGPUCompareFunction GetCompareFunc(const GLenum glCompareFunc, bool testEnabled)
{
    if (!testEnabled)
    {
        return WGPUCompareFunction_Always;
    }

    switch (glCompareFunc)
    {
        case GL_NEVER:
            return WGPUCompareFunction_Never;
        case GL_LESS:
            return WGPUCompareFunction_Less;
        case GL_EQUAL:
            return WGPUCompareFunction_Equal;
        case GL_LEQUAL:
            return WGPUCompareFunction_LessEqual;
        case GL_GREATER:
            return WGPUCompareFunction_Greater;
        case GL_NOTEQUAL:
            return WGPUCompareFunction_NotEqual;
        case GL_GEQUAL:
            return WGPUCompareFunction_GreaterEqual;
        case GL_ALWAYS:
            return WGPUCompareFunction_Always;
        default:
            UNREACHABLE();
            return WGPUCompareFunction_Always;
    }
}

wgpu::TextureSampleType GetTextureSampleType(gl::SamplerFormat samplerFormat)
{
    switch (samplerFormat)
    {
        case gl::SamplerFormat::Float:
            return wgpu::TextureSampleType::Float;
        case gl::SamplerFormat::Unsigned:
            return wgpu::TextureSampleType::Uint;
        case gl::SamplerFormat::Signed:
            return wgpu::TextureSampleType::Sint;
        case gl::SamplerFormat::Shadow:
            return wgpu::TextureSampleType::Depth;
        default:
            UNIMPLEMENTED();
            return wgpu::TextureSampleType::Undefined;
    }
}

WGPUStencilOperation GetStencilOp(const GLenum glStencilOp)
{
    switch (glStencilOp)
    {
        case GL_KEEP:
            return WGPUStencilOperation_Keep;
        case GL_ZERO:
            return WGPUStencilOperation_Zero;
        case GL_REPLACE:
            return WGPUStencilOperation_Replace;
        case GL_INCR:
            return WGPUStencilOperation_IncrementClamp;
        case GL_DECR:
            return WGPUStencilOperation_DecrementClamp;
        case GL_INCR_WRAP:
            return WGPUStencilOperation_IncrementWrap;
        case GL_DECR_WRAP:
            return WGPUStencilOperation_DecrementWrap;
        case GL_INVERT:
            return WGPUStencilOperation_Invert;
        default:
            UNREACHABLE();
            return WGPUStencilOperation_Keep;
    }
}

wgpu::FilterMode GetFilter(const GLenum filter)
{
    switch (filter)
    {
        case GL_LINEAR_MIPMAP_LINEAR:
        case GL_LINEAR_MIPMAP_NEAREST:
        case GL_LINEAR:
            return wgpu::FilterMode::Linear;
        case GL_NEAREST_MIPMAP_LINEAR:
        case GL_NEAREST_MIPMAP_NEAREST:
        case GL_NEAREST:
            return wgpu::FilterMode::Nearest;
        default:
            UNREACHABLE();
            return wgpu::FilterMode::Undefined;
    }
}

wgpu::MipmapFilterMode GetSamplerMipmapMode(const GLenum filter)
{
    switch (filter)
    {
        case GL_LINEAR_MIPMAP_LINEAR:
        case GL_NEAREST_MIPMAP_LINEAR:
            return wgpu::MipmapFilterMode::Linear;
        // GL_LINEAR and GL_NEAREST do not map directly to WebGPU but can be easily emulated,
        // see below.
        case GL_LINEAR:
        case GL_NEAREST:
        case GL_NEAREST_MIPMAP_NEAREST:
        case GL_LINEAR_MIPMAP_NEAREST:
            return wgpu::MipmapFilterMode::Nearest;
        default:
            UNREACHABLE();
            return wgpu::MipmapFilterMode::Undefined;
    }
}

wgpu::AddressMode GetSamplerAddressMode(const GLenum wrap)
{
    switch (wrap)
    {
        case GL_REPEAT:
            return wgpu::AddressMode::Repeat;
        case GL_MIRRORED_REPEAT:
            return wgpu::AddressMode::MirrorRepeat;
        case GL_CLAMP_TO_BORDER:
            // Not in WebGPU and not available in ES 3.0 or before.
            UNIMPLEMENTED();
            return wgpu::AddressMode::ClampToEdge;
        case GL_CLAMP_TO_EDGE:
            return wgpu::AddressMode::ClampToEdge;
        case GL_MIRROR_CLAMP_TO_EDGE_EXT:
            // Not in WebGPU and not available in ES 3.0 or before.
            return wgpu::AddressMode::ClampToEdge;
        default:
            UNREACHABLE();
            return wgpu::AddressMode::Undefined;
    }
}

wgpu::CompareFunction GetSamplerCompareFunc(const gl::SamplerState *samplerState)
{
    if (samplerState->getCompareMode() != GL_COMPARE_REF_TO_TEXTURE)
    {
        return wgpu::CompareFunction::Undefined;
    }

    return static_cast<wgpu::CompareFunction>(
        GetCompareFunc(samplerState->getCompareFunc(), /*testEnabled=*/true));
}

wgpu::SamplerDescriptor GetWgpuSamplerDesc(const gl::SamplerState *samplerState)
{
    wgpu::MipmapFilterMode wgpuMipmapFilterMode =
        GetSamplerMipmapMode(samplerState->getMinFilter());
    // Negative values don't seem to make a difference to the behavior of GLES, a min lod of 0.0
    // functions the same.
    float wgpuLodMinClamp =
        gl::clamp(samplerState->getMinLod(), webgpu::kWGPUMinLod, webgpu::kWGPUMaxLod);
    float wgpuLodMaxClamp =
        gl::clamp(samplerState->getMaxLod(), webgpu::kWGPUMinLod, webgpu::kWGPUMaxLod);

    if (!gl::IsMipmapFiltered(samplerState->getMinFilter()))
    {
        // Similarly to Vulkan, GL_NEAREST and GL_LINEAR do not map directly to WGPU, so
        // they must be emulated (See "Mapping of OpenGL to Vulkan filter modes")
        wgpuMipmapFilterMode = wgpu::MipmapFilterMode::Nearest;
        wgpuLodMinClamp      = 0.0f;
        wgpuLodMaxClamp      = 0.25f;
    }

    wgpu::SamplerDescriptor samplerDesc;
    samplerDesc.addressModeU = GetSamplerAddressMode(samplerState->getWrapS());
    samplerDesc.addressModeV = GetSamplerAddressMode(samplerState->getWrapT());
    samplerDesc.addressModeW = GetSamplerAddressMode(samplerState->getWrapR());
    samplerDesc.magFilter    = GetFilter(samplerState->getMagFilter());
    samplerDesc.minFilter    = GetFilter(samplerState->getMinFilter());
    samplerDesc.mipmapFilter = wgpuMipmapFilterMode;
    samplerDesc.lodMinClamp  = wgpuLodMinClamp;
    samplerDesc.lodMaxClamp  = wgpuLodMaxClamp;
    samplerDesc.compare      = GetSamplerCompareFunc(samplerState);
    // TODO(anglebug.com/389145696): there's no way to get the supported maxAnisotropy value from
    // WGPU, so there's no way to communicate to the GL client whether anisotropy is even supported
    // as an extension, let alone what the max value is.
    samplerDesc.maxAnisotropy = static_cast<uint16_t>(std::floor(samplerState->getMaxAnisotropy()));

    return samplerDesc;
}

uint32_t GetFirstIndexForDrawCall(gl::DrawElementsType indexType, const void *indices)
{
    const size_t indexSize                = gl::GetDrawElementsTypeSize(indexType);
    const uintptr_t indexBufferByteOffset = reinterpret_cast<uintptr_t>(indices);
    if (indexBufferByteOffset % indexSize != 0)
    {
        // WebGPU only allows offsetting index buffers by multiples of the index size
        UNIMPLEMENTED();
    }

    return static_cast<uint32_t>(indexBufferByteOffset / indexSize);
}

}  // namespace gl_wgpu
}  // namespace rx
