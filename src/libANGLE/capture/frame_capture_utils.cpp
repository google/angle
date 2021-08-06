//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// frame_capture_utils.cpp:
//   ANGLE frame capture util implementation.
//

#include "libANGLE/capture/frame_capture_utils.h"

#include <vector>

#include "common/Color.h"
#include "common/MemoryBuffer.h"
#include "common/angleutils.h"

#include "libANGLE/capture/gl_enum_utils.h"

#include "libANGLE/Buffer.h"
#include "libANGLE/Caps.h"
#include "libANGLE/Context.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/Query.h"
#include "libANGLE/RefCountObject.h"
#include "libANGLE/ResourceMap.h"
#include "libANGLE/Sampler.h"
#include "libANGLE/State.h"

#include "libANGLE/TransformFeedback.h"
#include "libANGLE/VertexAttribute.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/renderer/FramebufferImpl.h"
#include "libANGLE/renderer/RenderbufferImpl.h"
#include "libANGLE/serializer/JsonSerializer.h"

#if !ANGLE_CAPTURE_ENABLED
#    error Frame capture must be enabled to build this file.
#endif  // !ANGLE_CAPTURE_ENABLED

// Note: when diagnosing serialization comparison failures, you can disable the unused function
// compiler warning to allow bisecting the comparison function. One first check is to disable
// Framebuffer Attachment pixel comparison which includes the pixel contents of the default FBO.
// ANGLE_DISABLE_UNUSED_FUNCTION_WARNING

namespace angle
{

namespace
{

static const char *TextureTypeToString(gl::TextureType type)
{
    switch (type)
    {
        case gl::TextureType::_2D:
            return "TEXTURE_2D";
        case gl::TextureType::_2DArray:
            return "TEXTURE_2D_ARRAY";
        case gl::TextureType::_2DMultisample:
            return "TEXTURE_2DMS";
        case gl::TextureType::_2DMultisampleArray:
            return "TEXTURE_2DMS_ARRAY";
        case gl::TextureType::_3D:
            return "TEXTURE_3D";
        case gl::TextureType::External:
            return "TEXTURE_EXTERNAL";
        case gl::TextureType::Rectangle:
            return "TEXTURE_RECT";
        case gl::TextureType::CubeMap:
            return "TEXTURE_CUBE_MAP";
        case gl::TextureType::CubeMapArray:
            return "TEXTURE_CUBE_MAP_ARRAY";
        case gl::TextureType::VideoImage:
            return "TEXTURE_VIDEO_IMAGE";
        case gl::TextureType::Buffer:
            return "TEXTURE_BUFFER";
        default:
            return "invalid";
    }
}

static const char *CullFaceModeToString(gl::CullFaceMode mode)
{
    switch (mode)
    {
        case gl::CullFaceMode::Back:
            return "CULL_BACK";
        case gl::CullFaceMode::Front:
            return "CULL_FRONT";
        case gl::CullFaceMode::FrontAndBack:
            return "CULL_FRONT_AND_BACK";
        default:
            return "invalid";
    }
}

static const char *ProvokingVertexConventionToString(gl::ProvokingVertexConvention mode)
{
    switch (mode)
    {
        case gl::ProvokingVertexConvention::FirstVertexConvention:
            return "First";
        case gl::ProvokingVertexConvention::LastVertexConvention:
            return "Last";
        default:
            return "invalid";
    }
}

static const char *InitStateToString(gl::InitState state)
{
    return state == gl::InitState::Initialized ? "Initialized" : "MayNeedInit";
}

static const char *BlockLayoutTypeToString(sh::BlockLayoutType type)
{
    switch (type)
    {
        case sh::BlockLayoutType::BLOCKLAYOUT_STD140:
            return "std140";
        case sh::BlockLayoutType::BLOCKLAYOUT_STD430:
            return "std430";
        case sh::BlockLayoutType::BLOCKLAYOUT_PACKED:
            return "packed";
        case sh::BlockLayoutType::BLOCKLAYOUT_SHARED:
            return "shared";
        default:
            return "invalid";
    }
}

static const char *BlockTypeToString(sh::BlockType type)
{
    return type == sh::BlockType::BLOCK_BUFFER ? "buffer" : "uniform";
}

static const char *InterpolationTypeToString(sh::InterpolationType type)
{
    switch (type)
    {
        case sh::InterpolationType::INTERPOLATION_SMOOTH:
            return "smooth";
        case sh::InterpolationType::INTERPOLATION_CENTROID:
            return "centroid";
        case sh::InterpolationType::INTERPOLATION_SAMPLE:
            return "sample";
        case sh::InterpolationType::INTERPOLATION_FLAT:
            return "flat";
        case sh::InterpolationType::INTERPOLATION_NOPERSPECTIVE:
            return "noperspective";
        default:
            return "invalid";
    }
}

#define ENUM_TO_STRING(C, M) \
    case C ::M:              \
        return #M

static const char *PrimitiveModeToString(gl::PrimitiveMode mode)
{
    switch (mode)
    {
        ENUM_TO_STRING(gl::PrimitiveMode, Points);
        ENUM_TO_STRING(gl::PrimitiveMode, Lines);
        ENUM_TO_STRING(gl::PrimitiveMode, LineLoop);
        ENUM_TO_STRING(gl::PrimitiveMode, LineStrip);
        ENUM_TO_STRING(gl::PrimitiveMode, Triangles);
        ENUM_TO_STRING(gl::PrimitiveMode, TriangleStrip);
        ENUM_TO_STRING(gl::PrimitiveMode, TriangleFan);
        ENUM_TO_STRING(gl::PrimitiveMode, Unused1);
        ENUM_TO_STRING(gl::PrimitiveMode, Unused2);
        ENUM_TO_STRING(gl::PrimitiveMode, Unused3);
        ENUM_TO_STRING(gl::PrimitiveMode, LinesAdjacency);
        ENUM_TO_STRING(gl::PrimitiveMode, LineStripAdjacency);
        ENUM_TO_STRING(gl::PrimitiveMode, TrianglesAdjacency);
        ENUM_TO_STRING(gl::PrimitiveMode, TriangleStripAdjacency);
        ENUM_TO_STRING(gl::PrimitiveMode, Patches);
        default:
            return "invalid";
    }
}

static const char *BufferUsageToString(gl::BufferUsage usage)
{
    switch (usage)
    {
        ENUM_TO_STRING(gl::BufferUsage, DynamicCopy);
        ENUM_TO_STRING(gl::BufferUsage, DynamicDraw);
        ENUM_TO_STRING(gl::BufferUsage, DynamicRead);
        ENUM_TO_STRING(gl::BufferUsage, StaticCopy);
        ENUM_TO_STRING(gl::BufferUsage, StaticDraw);
        ENUM_TO_STRING(gl::BufferUsage, StaticRead);
        ENUM_TO_STRING(gl::BufferUsage, StreamCopy);
        ENUM_TO_STRING(gl::BufferUsage, StreamDraw);
        ENUM_TO_STRING(gl::BufferUsage, StreamRead);
        default:
            return "invalid";
    }
}

static const char *SrgbOverrideToString(gl::SrgbOverride value)
{
    switch (value)
    {
        ENUM_TO_STRING(gl::SrgbOverride, Default);
        ENUM_TO_STRING(gl::SrgbOverride, SRGB);
        ENUM_TO_STRING(gl::SrgbOverride, Linear);
        default:
            return "invalid";
    }
}

static const char *ColorGenericTypeToString(gl::ColorGeneric::Type type)
{
    switch (type)
    {
        ENUM_TO_STRING(gl::ColorGeneric::Type, Float);
        ENUM_TO_STRING(gl::ColorGeneric::Type, Int);
        ENUM_TO_STRING(gl::ColorGeneric::Type, UInt);
        default:
            return "invalid";
    }
}

static const char *CompileStatusToString(gl::CompileStatus status)
{
    switch (status)
    {
        ENUM_TO_STRING(gl::CompileStatus, NOT_COMPILED);
        ENUM_TO_STRING(gl::CompileStatus, COMPILE_REQUESTED);
        ENUM_TO_STRING(gl::CompileStatus, COMPILED);
        default:
            return "invalid";
    }
}

#undef ENUM_TO_STRING

class ANGLE_NO_DISCARD GroupScope
{
  public:
    GroupScope(JsonSerializer *json, const std::string &name) : mJson(json)
    {
        mJson->startGroup(name);
    }

    GroupScope(JsonSerializer *json, const std::string &name, int index) : mJson(json)
    {
        constexpr size_t kBufSize = 255;
        char buf[kBufSize + 1]    = {};
        snprintf(buf, kBufSize, "%s%s%03d", name.c_str(), name.empty() ? "" : " ", index);
        mJson->startGroup(buf);
    }

    GroupScope(JsonSerializer *json, int index) : GroupScope(json, "", index) {}

    ~GroupScope() { mJson->endGroup(); }

  private:
    JsonSerializer *mJson;
};

void SerializeColorF(JsonSerializer *json, const ColorF &color)
{
    json->addScalar("red", color.red);
    json->addScalar("green", color.green);
    json->addScalar("blue", color.blue);
    json->addScalar("alpha", color.alpha);
}

void SerializeColorFWithGroup(JsonSerializer *json, const char *groupName, const ColorF &color)
{
    GroupScope group(json, groupName);
    SerializeColorF(json, color);
}

void SerializeColorI(JsonSerializer *json, const ColorI &color)
{
    json->addScalar("Red", color.red);
    json->addScalar("Green", color.green);
    json->addScalar("Blue", color.blue);
    json->addScalar("Alpha", color.alpha);
}

void SerializeColorUI(JsonSerializer *json, const ColorUI &color)
{
    json->addScalar("Red", color.red);
    json->addScalar("Green", color.green);
    json->addScalar("Blue", color.blue);
    json->addScalar("Alpha", color.alpha);
}

void SerializeExtents(JsonSerializer *json, const gl::Extents &extents)
{
    json->addScalar("Width", extents.width);
    json->addScalar("Height", extents.height);
    json->addScalar("Depth", extents.depth);
}

template <class ObjectType>
void SerializeOffsetBindingPointerVector(
    JsonSerializer *json,
    const char *groupName,
    const std::vector<gl::OffsetBindingPointer<ObjectType>> &offsetBindingPointerVector)
{
    GroupScope vectorGroup(json, groupName);

    for (size_t i = 0; i < offsetBindingPointerVector.size(); i++)
    {
        GroupScope itemGroup(json, static_cast<int>(i));
        json->addScalar("Value", offsetBindingPointerVector[i].id().value);
        json->addScalar("Offset", offsetBindingPointerVector[i].getOffset());
        json->addScalar("Size", offsetBindingPointerVector[i].getSize());
    }
}

template <class ObjectType>
void SerializeBindingPointerVector(
    JsonSerializer *json,
    const std::vector<gl::BindingPointer<ObjectType>> &bindingPointerVector)
{
    for (size_t i = 0; i < bindingPointerVector.size(); i++)
    {
        // Do not serialize zero bindings, as this will create unwanted diffs
        if (bindingPointerVector[i].id().value != 0)
        {
            std::ostringstream s;
            s << i;
            json->addScalar(s.str().c_str(), bindingPointerVector[i].id().value);
        }
    }
}

template <class T>
void SerializeRange(JsonSerializer *json, const gl::Range<T> &range)
{
    GroupScope group(json, "Range");
    json->addScalar("Low", range.low());
    json->addScalar("High", range.high());
}

bool IsValidColorAttachmentBinding(GLenum binding, size_t colorAttachmentsCount)
{
    return binding == GL_BACK || (binding >= GL_COLOR_ATTACHMENT0 &&
                                  (binding - GL_COLOR_ATTACHMENT0) < colorAttachmentsCount);
}

Result ReadPixelsFromAttachment(const gl::Context *context,
                                gl::Framebuffer *framebuffer,
                                const gl::FramebufferAttachment &framebufferAttachment,
                                ScratchBuffer *scratchBuffer,
                                MemoryBuffer **pixels)
{
    gl::Extents extents       = framebufferAttachment.getSize();
    GLenum binding            = framebufferAttachment.getBinding();
    gl::InternalFormat format = *framebufferAttachment.getFormat().info;
    if (IsValidColorAttachmentBinding(binding,
                                      framebuffer->getState().getColorAttachments().size()))
    {
        format = framebuffer->getImplementation()->getImplementationColorReadFormat(context);
    }
    ANGLE_CHECK_GL_ALLOC(const_cast<gl::Context *>(context),
                         scratchBuffer->getInitialized(
                             format.pixelBytes * extents.width * extents.height, pixels, 0));
    ANGLE_TRY(framebuffer->readPixels(context, gl::Rectangle{0, 0, extents.width, extents.height},
                                      format.format, format.type, gl::PixelPackState{}, nullptr,
                                      (*pixels)->data()));
    return Result::Continue;
}
void SerializeImageIndex(JsonSerializer *json, const gl::ImageIndex &imageIndex)
{
    GroupScope group(json, "Image");
    json->addCString("ImageType", TextureTypeToString(imageIndex.getType()));
    json->addScalar("LevelIndex", imageIndex.getLevelIndex());
    json->addScalar("LayerIndex", imageIndex.getLayerIndex());
    json->addScalar("LayerCount", imageIndex.getLayerCount());
}

Result SerializeFramebufferAttachment(const gl::Context *context,
                                      JsonSerializer *json,
                                      ScratchBuffer *scratchBuffer,
                                      gl::Framebuffer *framebuffer,
                                      const gl::FramebufferAttachment &framebufferAttachment,
                                      gl::GLenumGroup enumGroup)
{
    if (framebufferAttachment.type() == GL_TEXTURE ||
        framebufferAttachment.type() == GL_RENDERBUFFER)
    {
        json->addScalar("ID", framebufferAttachment.id());
    }
    json->addScalar("Type", framebufferAttachment.type());
    // serialize target variable
    json->addString("Binding", gl::GLenumToString(enumGroup, framebufferAttachment.getBinding()));
    if (framebufferAttachment.type() == GL_TEXTURE)
    {
        SerializeImageIndex(json, framebufferAttachment.getTextureImageIndex());
    }
    json->addScalar("NumViews", framebufferAttachment.getNumViews());
    json->addScalar("Multiview", framebufferAttachment.isMultiview());
    json->addScalar("ViewIndex", framebufferAttachment.getBaseViewIndex());
    json->addScalar("Samples", framebufferAttachment.getRenderToTextureSamples());

    {
        GroupScope extentsGroup(json, "Extents");
        SerializeExtents(json, framebufferAttachment.getSize());
    }

    if (framebufferAttachment.type() != GL_TEXTURE &&
        framebufferAttachment.type() != GL_RENDERBUFFER)
    {
        GLenum prevReadBufferState = framebuffer->getReadBufferState();
        GLenum binding             = framebufferAttachment.getBinding();
        if (IsValidColorAttachmentBinding(binding,
                                          framebuffer->getState().getColorAttachments().size()))
        {
            framebuffer->setReadBuffer(framebufferAttachment.getBinding());
            ANGLE_TRY(framebuffer->syncState(context, GL_FRAMEBUFFER, gl::Command::Other));
        }

        if (framebufferAttachment.initState() == gl::InitState::Initialized)
        {
            MemoryBuffer *pixelsPtr = nullptr;
            ANGLE_TRY(ReadPixelsFromAttachment(context, framebuffer, framebufferAttachment,
                                               scratchBuffer, &pixelsPtr));
            json->addBlob("Data", pixelsPtr->data(), pixelsPtr->size());
        }
        else
        {
            json->addCString("Data", "Not initialized");
        }
        // Reset framebuffer state
        framebuffer->setReadBuffer(prevReadBufferState);
    }
    return Result::Continue;
}

Result SerializeFramebufferState(const gl::Context *context,
                                 JsonSerializer *json,
                                 ScratchBuffer *scratchBuffer,
                                 gl::Framebuffer *framebuffer,
                                 const gl::FramebufferState &framebufferState)
{
    GroupScope group(json, "Framebuffer", framebufferState.id().value);

    json->addString("Label", framebufferState.getLabel());
    json->addVector("DrawStates", framebufferState.getDrawBufferStates());
    json->addScalar("ReadBufferState", framebufferState.getReadBufferState());
    json->addScalar("DefaultWidth", framebufferState.getDefaultWidth());
    json->addScalar("DefaultHeight", framebufferState.getDefaultHeight());
    json->addScalar("DefaultSamples", framebufferState.getDefaultSamples());
    json->addScalar("DefaultFixedSampleLocation",
                    framebufferState.getDefaultFixedSampleLocations());
    json->addScalar("DefaultLayers", framebufferState.getDefaultLayers());

    const std::vector<gl::FramebufferAttachment> &colorAttachments =
        framebufferState.getColorAttachments();
    for (const gl::FramebufferAttachment &colorAttachment : colorAttachments)
    {
        if (colorAttachment.isAttached())
        {
            GroupScope colorAttachmentgroup(json, "ColorAttachment");
            ANGLE_TRY(SerializeFramebufferAttachment(context, json, scratchBuffer, framebuffer,
                                                     colorAttachment,
                                                     gl::GLenumGroup::ColorBuffer));
        }
    }
    if (framebuffer->getDepthStencilAttachment())
    {
        GroupScope dsAttachmentgroup(json, "DepthStencilAttachment");
        ANGLE_TRY(SerializeFramebufferAttachment(context, json, scratchBuffer, framebuffer,
                                                 *framebuffer->getDepthStencilAttachment(),
                                                 gl::GLenumGroup::DefaultGroup));
    }
    else
    {
        if (framebuffer->getDepthAttachment())
        {
            GroupScope depthAttachmentgroup(json, "DepthAttachment");
            ANGLE_TRY(SerializeFramebufferAttachment(context, json, scratchBuffer, framebuffer,
                                                     *framebuffer->getDepthAttachment(),
                                                     gl::GLenumGroup::FramebufferAttachment));
        }
        if (framebuffer->getStencilAttachment())
        {
            GroupScope stencilAttachmengroup(json, "StencilAttachment");
            ANGLE_TRY(SerializeFramebufferAttachment(context, json, scratchBuffer, framebuffer,
                                                     *framebuffer->getStencilAttachment(),
                                                     gl::GLenumGroup::DefaultGroup));
        }
    }
    return Result::Continue;
}

Result SerializeFramebuffer(const gl::Context *context,
                            JsonSerializer *json,
                            ScratchBuffer *scratchBuffer,
                            gl::Framebuffer *framebuffer)
{
    return SerializeFramebufferState(context, json, scratchBuffer, framebuffer,
                                     framebuffer->getState());
}

void SerializeRasterizerState(JsonSerializer *json, const gl::RasterizerState &rasterizerState)
{
    GroupScope group(json, "Rasterizer");
    json->addScalar("CullFace", rasterizerState.cullFace);
    json->addCString("CullMode", CullFaceModeToString(rasterizerState.cullMode));
    json->addScalar("FrontFace", rasterizerState.frontFace);
    json->addScalar("PolygonOffsetFill", rasterizerState.polygonOffsetFill);
    json->addScalar("PolygonOffsetFactor", rasterizerState.polygonOffsetFactor);
    json->addScalar("PolygonOffsetUnits", rasterizerState.polygonOffsetUnits);
    json->addScalar("PointDrawMode", rasterizerState.pointDrawMode);
    json->addScalar("MultiSample", rasterizerState.multiSample);
    json->addScalar("RasterizerDiscard", rasterizerState.rasterizerDiscard);
    json->addScalar("Dither", rasterizerState.dither);
}

void SerializeRectangle(JsonSerializer *json,
                        const std::string &name,
                        const gl::Rectangle &rectangle)
{
    GroupScope group(json, name);
    json->addScalar("x", rectangle.x);
    json->addScalar("y", rectangle.y);
    json->addScalar("w", rectangle.width);
    json->addScalar("h", rectangle.height);
}

void SerializeBlendStateExt(JsonSerializer *json, const gl::BlendStateExt &blendStateExt)
{
    GroupScope group(json, "BlendStateExt");
    json->addScalar("MaxDrawBuffers", blendStateExt.mMaxDrawBuffers);
    json->addScalar("enableMask", blendStateExt.mEnabledMask.bits());
    json->addScalar("DstColor", blendStateExt.mDstColor);
    json->addScalar("DstAlpha", blendStateExt.mDstAlpha);
    json->addScalar("SrcColor", blendStateExt.mSrcColor);
    json->addScalar("SrcAlpha", blendStateExt.mSrcAlpha);
    json->addScalar("EquationColor", blendStateExt.mEquationColor);
    json->addScalar("EquationAlpha", blendStateExt.mEquationAlpha);
    json->addScalar("ColorMask", blendStateExt.mColorMask);
}

void SerializeDepthStencilState(JsonSerializer *json,
                                const gl::DepthStencilState &depthStencilState)
{
    GroupScope group(json, "DepthStencilState");
    json->addScalar("DepthTest", depthStencilState.depthTest);
    json->addScalar("DepthFunc", depthStencilState.depthFunc);
    json->addScalar("DepthMask", depthStencilState.depthMask);
    json->addScalar("StencilTest", depthStencilState.stencilTest);
    json->addScalar("StencilFunc", depthStencilState.stencilFunc);
    json->addScalar("StencilMask", depthStencilState.stencilMask);
    json->addScalar("StencilFail", depthStencilState.stencilFail);
    json->addScalar("StencilPassDepthFail", depthStencilState.stencilPassDepthFail);
    json->addScalar("StencilPassDepthPass", depthStencilState.stencilPassDepthPass);
    json->addScalar("StencilWritemask", depthStencilState.stencilWritemask);
    json->addScalar("StencilBackFunc", depthStencilState.stencilBackFunc);
    json->addScalar("StencilBackMask", depthStencilState.stencilBackMask);
    json->addScalar("StencilBackFail", depthStencilState.stencilBackFail);
    json->addScalar("StencilBackPassDepthFail", depthStencilState.stencilBackPassDepthFail);
    json->addScalar("StencilBackPassDepthPass", depthStencilState.stencilBackPassDepthPass);
    json->addScalar("StencilBackWritemask", depthStencilState.stencilBackWritemask);
}

void SerializeVertexAttribCurrentValueData(
    JsonSerializer *json,
    const gl::VertexAttribCurrentValueData &vertexAttribCurrentValueData)
{
    ASSERT(vertexAttribCurrentValueData.Type == gl::VertexAttribType::Float ||
           vertexAttribCurrentValueData.Type == gl::VertexAttribType::Int ||
           vertexAttribCurrentValueData.Type == gl::VertexAttribType::UnsignedInt);
    if (vertexAttribCurrentValueData.Type == gl::VertexAttribType::Float)
    {
        json->addScalar("0", vertexAttribCurrentValueData.Values.FloatValues[0]);
        json->addScalar("1", vertexAttribCurrentValueData.Values.FloatValues[1]);
        json->addScalar("2", vertexAttribCurrentValueData.Values.FloatValues[2]);
        json->addScalar("3", vertexAttribCurrentValueData.Values.FloatValues[3]);
    }
    else if (vertexAttribCurrentValueData.Type == gl::VertexAttribType::Int)
    {
        json->addScalar("0", vertexAttribCurrentValueData.Values.IntValues[0]);
        json->addScalar("1", vertexAttribCurrentValueData.Values.IntValues[1]);
        json->addScalar("2", vertexAttribCurrentValueData.Values.IntValues[2]);
        json->addScalar("3", vertexAttribCurrentValueData.Values.IntValues[3]);
    }
    else
    {
        json->addScalar("0", vertexAttribCurrentValueData.Values.UnsignedIntValues[0]);
        json->addScalar("1", vertexAttribCurrentValueData.Values.UnsignedIntValues[1]);
        json->addScalar("2", vertexAttribCurrentValueData.Values.UnsignedIntValues[2]);
        json->addScalar("3", vertexAttribCurrentValueData.Values.UnsignedIntValues[3]);
    }
}

void SerializePixelPackState(JsonSerializer *json, const gl::PixelPackState &pixelPackState)
{
    GroupScope group(json, "PixelPackState");
    json->addScalar("Alignment", pixelPackState.alignment);
    json->addScalar("RowLength", pixelPackState.rowLength);
    json->addScalar("SkipRows", pixelPackState.skipRows);
    json->addScalar("SkipPixels", pixelPackState.skipPixels);
    json->addScalar("ImageHeight", pixelPackState.imageHeight);
    json->addScalar("SkipImages", pixelPackState.skipImages);
    json->addScalar("ReverseRowOrder", pixelPackState.reverseRowOrder);
}

void SerializePixelUnpackState(JsonSerializer *json, const gl::PixelUnpackState &pixelUnpackState)
{
    GroupScope group(json, "PixelUnpackState");
    json->addScalar("Alignment", pixelUnpackState.alignment);
    json->addScalar("RowLength", pixelUnpackState.rowLength);
    json->addScalar("SkipRows", pixelUnpackState.skipRows);
    json->addScalar("SkipPixels", pixelUnpackState.skipPixels);
    json->addScalar("ImageHeight", pixelUnpackState.imageHeight);
    json->addScalar("SkipImages", pixelUnpackState.skipImages);
}

void SerializeImageUnit(JsonSerializer *json, const gl::ImageUnit &imageUnit)
{
    GroupScope group(json, "ImageUnit");
    json->addScalar("Level", imageUnit.level);
    json->addScalar("Layered", imageUnit.layered);
    json->addScalar("Layer", imageUnit.layer);
    json->addScalar("Access", imageUnit.access);
    json->addScalar("Format", imageUnit.format);
    json->addScalar("Texid", imageUnit.texture.id().value);
}

void SerializeContextState(JsonSerializer *json, const gl::State &state)
{
    GroupScope group(json, "ContextState");
    json->addScalar("ClientType", state.getClientType());
    json->addScalar("Priority", state.getContextPriority());
    json->addScalar("Major", state.getClientMajorVersion());
    json->addScalar("Minor", state.getClientMinorVersion());
    SerializeColorFWithGroup(json, "ColorClearValue", state.getColorClearValue());
    json->addScalar("DepthClearValue", state.getDepthClearValue());
    json->addScalar("StencilClearValue", state.getStencilClearValue());
    SerializeRasterizerState(json, state.getRasterizerState());
    json->addScalar("ScissorTestEnabled", state.isScissorTestEnabled());
    SerializeRectangle(json, "Scissors", state.getScissor());
    SerializeBlendStateExt(json, state.getBlendStateExt());
    SerializeColorFWithGroup(json, "BlendColor", state.getBlendColor());
    json->addScalar("SampleAlphaToCoverageEnabled", state.isSampleAlphaToCoverageEnabled());
    json->addScalar("SampleCoverageEnabled", state.isSampleCoverageEnabled());
    json->addScalar("SampleCoverageValue", state.getSampleCoverageValue());
    json->addScalar("SampleCoverageInvert", state.getSampleCoverageInvert());
    json->addScalar("SampleMaskEnabled", state.isSampleMaskEnabled());
    json->addScalar("MaxSampleMaskWords", state.getMaxSampleMaskWords());
    {
        const auto &sampleMaskValues = state.getSampleMaskValues();
        GroupScope maskGroup(json, "SampleMaskValues");
        for (size_t i = 0; i < sampleMaskValues.size(); i++)
        {
            std::ostringstream os;
            os << i;
            json->addScalar(os.str(), sampleMaskValues[i]);
        }
    }
    SerializeDepthStencilState(json, state.getDepthStencilState());
    json->addScalar("StencilRef", state.getStencilRef());
    json->addScalar("StencilBackRef", state.getStencilBackRef());
    json->addScalar("LineWidth", state.getLineWidth());
    json->addScalar("GenerateMipmapHint", state.getGenerateMipmapHint());
    json->addScalar("TextureFilteringHint", state.getTextureFilteringHint());
    json->addScalar("FragmentShaderDerivativeHint", state.getFragmentShaderDerivativeHint());
    json->addScalar("BindGeneratesResourceEnabled", state.isBindGeneratesResourceEnabled());
    json->addScalar("ClientArraysEnabled", state.areClientArraysEnabled());
    SerializeRectangle(json, "Viewport", state.getViewport());
    json->addScalar("Near", state.getNearPlane());
    json->addScalar("Far", state.getFarPlane());
    if (state.getReadFramebuffer())
    {
        json->addScalar("Framebuffer ID", state.getReadFramebuffer()->id().value);
    }
    if (state.getDrawFramebuffer())
    {
        json->addScalar("Draw Framebuffer ID", state.getDrawFramebuffer()->id().value);
    }
    json->addScalar("Renderbuffer ID", state.getRenderbufferId().value);
    if (state.getProgram())
    {
        json->addScalar("ProgramID", state.getProgram()->id().value);
    }
    if (state.getProgramPipeline())
    {
        json->addScalar("ProgramPipelineID", state.getProgramPipeline()->id().value);
    }
    json->addCString("ProvokingVertex",
                     ProvokingVertexConventionToString(state.getProvokingVertex()));
    const std::vector<gl::VertexAttribCurrentValueData> &vertexAttribCurrentValues =
        state.getVertexAttribCurrentValues();
    for (size_t i = 0; i < vertexAttribCurrentValues.size(); i++)
    {
        GroupScope vagroup(json, "VertexAttribCurrentValue", static_cast<int>(i));
        SerializeVertexAttribCurrentValueData(json, vertexAttribCurrentValues[i]);
    }
    if (state.getVertexArray())
    {
        json->addScalar("VertexArrayID", state.getVertexArray()->id().value);
    }
    json->addScalar("CurrentValuesTypeMask", state.getCurrentValuesTypeMask().to_ulong());
    json->addScalar("ActiveSampler", state.getActiveSampler());
    {
        GroupScope boundTexturesGroup(json, "BoundTextures");
        for (const auto &textures : state.getBoundTexturesForCapture())
        {
            SerializeBindingPointerVector<gl::Texture>(json, textures);
        }
    }
    json->addScalar("TexturesIncompatibleWithSamplers",
                    state.getTexturesIncompatibleWithSamplers().to_ulong());
    SerializeBindingPointerVector<gl::Sampler>(json, state.getSamplers());

    {
        GroupScope imageUnitsGroup(json, "BoundImageUnits");
        for (const gl::ImageUnit &imageUnit : state.getImageUnits())
        {
            SerializeImageUnit(json, imageUnit);
        }
    }

    {
        const gl::ActiveQueryMap &activeQueries = state.getActiveQueriesForCapture();
        GroupScope activeQueriesGroup(json, "ActiveQueries");
        for (gl::QueryType queryType : AllEnums<gl::QueryType>())
        {
            const gl::BindingPointer<gl::Query> &query = activeQueries[queryType];
            std::stringstream strstr;
            strstr << queryType;
            json->addScalar(strstr.str(), query.id().value);
        }
    }

    {
        const gl::BoundBufferMap &boundBuffers = state.getBoundBuffersForCapture();
        GroupScope boundBuffersGroup(json, "BoundBuffers");
        for (gl::BufferBinding bufferBinding : AllEnums<gl::BufferBinding>())
        {
            const gl::BindingPointer<gl::Buffer> &buffer = boundBuffers[bufferBinding];
            std::stringstream strstr;
            strstr << bufferBinding;
            json->addScalar(strstr.str(), buffer.id().value);
        }
    }

    SerializeOffsetBindingPointerVector<gl::Buffer>(json, "UniformBufferBindings",
                                                    state.getOffsetBindingPointerUniformBuffers());
    SerializeOffsetBindingPointerVector<gl::Buffer>(
        json, "AtomicCounterBufferBindings", state.getOffsetBindingPointerAtomicCounterBuffers());
    SerializeOffsetBindingPointerVector<gl::Buffer>(
        json, "ShaderStorageBufferBindings", state.getOffsetBindingPointerShaderStorageBuffers());
    if (state.getCurrentTransformFeedback())
    {
        json->addScalar("CurrentTransformFeedback",
                        state.getCurrentTransformFeedback()->id().value);
    }
    SerializePixelUnpackState(json, state.getUnpackState());
    SerializePixelPackState(json, state.getPackState());
    json->addScalar("PrimitiveRestartEnabled", state.isPrimitiveRestartEnabled());
    json->addScalar("MultisamplingEnabled", state.isMultisamplingEnabled());
    json->addScalar("SampleAlphaToOneEnabled", state.isSampleAlphaToOneEnabled());
    json->addScalar("CoverageModulation", state.getCoverageModulation());
    json->addScalar("FramebufferSRGB", state.getFramebufferSRGB());
    json->addScalar("RobustResourceInitEnabled", state.isRobustResourceInitEnabled());
    json->addScalar("ProgramBinaryCacheEnabled", state.isProgramBinaryCacheEnabled());
    json->addScalar("TextureRectangleEnabled", state.isTextureRectangleEnabled());
    json->addScalar("MaxShaderCompilerThreads", state.getMaxShaderCompilerThreads());
    json->addScalar("EnabledClipDistances", state.getEnabledClipDistances().to_ulong());
    json->addScalar("BlendFuncConstantAlphaDrawBuffers",
                    state.getBlendFuncConstantAlphaDrawBuffers().to_ulong());
    json->addScalar("BlendFuncConstantColorDrawBuffers",
                    state.getBlendFuncConstantColorDrawBuffers().to_ulong());
    json->addScalar("SimultaneousConstantColorAndAlphaBlendFunc",
                    state.noSimultaneousConstantColorAndAlphaBlendFunc());
}

void SerializeBufferState(JsonSerializer *json, const gl::BufferState &bufferState)
{
    json->addString("Label", bufferState.getLabel());
    json->addCString("Usage", BufferUsageToString(bufferState.getUsage()));
    json->addScalar("Size", bufferState.getSize());
    json->addScalar("AccessFlags", bufferState.getAccessFlags());
    json->addScalar("Access", bufferState.getAccess());
    json->addScalar("Mapped", bufferState.isMapped());
    json->addScalar("MapOffset", bufferState.getMapOffset());
    json->addScalar("MapLength", bufferState.getMapLength());
}

Result SerializeBuffer(const gl::Context *context,
                       JsonSerializer *json,
                       ScratchBuffer *scratchBuffer,
                       gl::Buffer *buffer)
{
    GroupScope group(json, "Buffer", buffer->id().value);
    SerializeBufferState(json, buffer->getState());
    if (buffer->getSize())
    {
        MemoryBuffer *dataPtr = nullptr;
        ANGLE_CHECK_GL_ALLOC(
            const_cast<gl::Context *>(context),
            scratchBuffer->getInitialized(static_cast<size_t>(buffer->getSize()), &dataPtr, 0));
        ANGLE_TRY(buffer->getSubData(context, 0, dataPtr->size(), dataPtr->data()));
        json->addBlob("data", dataPtr->data(), dataPtr->size());
    }
    else
    {
        json->addCString("data", "null");
    }
    return Result::Continue;
}
void SerializeColorGeneric(JsonSerializer *json,
                           const std::string &name,
                           const ColorGeneric &colorGeneric)
{
    GroupScope group(json, name);
    ASSERT(colorGeneric.type == ColorGeneric::Type::Float ||
           colorGeneric.type == ColorGeneric::Type::Int ||
           colorGeneric.type == ColorGeneric::Type::UInt);
    json->addCString("Type", ColorGenericTypeToString(colorGeneric.type));
    if (colorGeneric.type == ColorGeneric::Type::Float)
    {
        SerializeColorF(json, colorGeneric.colorF);
    }
    else if (colorGeneric.type == ColorGeneric::Type::Int)
    {
        SerializeColorI(json, colorGeneric.colorI);
    }
    else
    {
        SerializeColorUI(json, colorGeneric.colorUI);
    }
}

void SerializeSamplerState(JsonSerializer *json, const gl::SamplerState &samplerState)
{
    json->addScalar("MinFilter", samplerState.getMinFilter());
    json->addScalar("MagFilter", samplerState.getMagFilter());
    json->addScalar("WrapS", samplerState.getWrapS());
    json->addScalar("WrapT", samplerState.getWrapT());
    json->addScalar("WrapR", samplerState.getWrapR());
    json->addScalar("MaxAnisotropy", samplerState.getMaxAnisotropy());
    json->addScalar("MinLod", samplerState.getMinLod());
    json->addScalar("MaxLod", samplerState.getMaxLod());
    json->addScalar("CompareMode", samplerState.getCompareMode());
    json->addScalar("CompareFunc", samplerState.getCompareFunc());
    json->addScalar("SRGBDecode", samplerState.getSRGBDecode());
    SerializeColorGeneric(json, "BorderColor", samplerState.getBorderColor());
}

void SerializeSampler(JsonSerializer *json, gl::Sampler *sampler)
{
    GroupScope group(json, "Sampler", sampler->id().value);
    json->addString("Label", sampler->getLabel());
    SerializeSamplerState(json, sampler->getSamplerState());
}

void SerializeSwizzleState(JsonSerializer *json, const gl::SwizzleState &swizzleState)
{
    json->addScalar("SwizzleRed", swizzleState.swizzleRed);
    json->addScalar("SwizzleGreen", swizzleState.swizzleGreen);
    json->addScalar("SwizzleBlue", swizzleState.swizzleBlue);
    json->addScalar("SwizzleAlpha", swizzleState.swizzleAlpha);
}

void SerializeInternalFormat(JsonSerializer *json, const gl::InternalFormat *internalFormat)
{
    json->addScalar("InternalFormat", internalFormat->internalFormat);
}

void SerializeFormat(JsonSerializer *json, const gl::Format &format)
{
    SerializeInternalFormat(json, format.info);
}

void SerializeRenderbufferState(JsonSerializer *json,
                                const gl::RenderbufferState &renderbufferState)
{
    GroupScope wg(json, "State");
    json->addScalar("Width", renderbufferState.getWidth());
    json->addScalar("Height", renderbufferState.getHeight());
    SerializeFormat(json, renderbufferState.getFormat());
    json->addScalar("Samples", renderbufferState.getSamples());
    json->addCString("InitState", InitStateToString(renderbufferState.getInitState()));
}

Result SerializeRenderbuffer(const gl::Context *context,
                             JsonSerializer *json,
                             ScratchBuffer *scratchBuffer,
                             gl::Renderbuffer *renderbuffer)
{
    GroupScope wg(json, "Renderbuffer", renderbuffer->id().value);
    SerializeRenderbufferState(json, renderbuffer->getState());
    json->addString("Label", renderbuffer->getLabel());
    MemoryBuffer *pixelsPtr = nullptr;
    ANGLE_CHECK_GL_ALLOC(
        const_cast<gl::Context *>(context),
        scratchBuffer->getInitialized(renderbuffer->getMemorySize(), &pixelsPtr, 0));

    if (renderbuffer->initState(gl::ImageIndex()) == gl::InitState::Initialized)
    {
        gl::PixelPackState packState;
        packState.alignment = 1;
        ANGLE_TRY(renderbuffer->getImplementation()->getRenderbufferImage(
            context, packState, nullptr, renderbuffer->getImplementationColorReadFormat(context),
            renderbuffer->getImplementationColorReadType(context), pixelsPtr->data()));
        json->addBlob("pixel", pixelsPtr->data(), pixelsPtr->size());
    }
    else
    {
        json->addCString("pixel", "Not initialized");
    }
    return Result::Continue;
}

void SerializeWorkGroupSize(JsonSerializer *json, const sh::WorkGroupSize &workGroupSize)
{
    GroupScope wg(json, "workGroupSize");
    json->addScalar("x", workGroupSize[0]);
    json->addScalar("y", workGroupSize[1]);
    json->addScalar("z", workGroupSize[2]);
}

void SerializeShaderVariable(JsonSerializer *json, const sh::ShaderVariable &shaderVariable)
{
    GroupScope wg(json, "ShaderVariable");
    json->addScalar("Type", shaderVariable.type);
    json->addScalar("Precision", shaderVariable.precision);
    json->addString("Name", shaderVariable.name);
    json->addString("MappedName", shaderVariable.mappedName);
    json->addVector("ArraySizes", shaderVariable.arraySizes);
    json->addScalar("StaticUse", shaderVariable.staticUse);
    json->addScalar("Active", shaderVariable.active);
    for (const sh::ShaderVariable &field : shaderVariable.fields)
    {
        SerializeShaderVariable(json, field);
    }
    json->addString("StructOrBlockName", shaderVariable.structOrBlockName);
    json->addString("MappedStructOrBlockName", shaderVariable.mappedStructOrBlockName);
    json->addScalar("RowMajorLayout", shaderVariable.isRowMajorLayout);
    json->addScalar("Location", shaderVariable.location);
    json->addScalar("Binding", shaderVariable.binding);
    json->addScalar("ImageUnitFormat", shaderVariable.imageUnitFormat);
    json->addScalar("Offset", shaderVariable.offset);
    json->addScalar("Readonly", shaderVariable.readonly);
    json->addScalar("Writeonly", shaderVariable.writeonly);
    json->addScalar("Index", shaderVariable.index);
    json->addScalar("YUV", shaderVariable.yuv);
    json->addCString("Interpolation", InterpolationTypeToString(shaderVariable.interpolation));
    json->addScalar("Invariant", shaderVariable.isInvariant);
    json->addScalar("TexelFetchStaticUse", shaderVariable.texelFetchStaticUse);
}

void SerializeShaderVariablesVector(JsonSerializer *json,
                                    const std::vector<sh::ShaderVariable> &shaderVariables)
{
    for (const sh::ShaderVariable &shaderVariable : shaderVariables)
    {
        SerializeShaderVariable(json, shaderVariable);
    }
}

void SerializeInterfaceBlocksVector(JsonSerializer *json,
                                    const std::vector<sh::InterfaceBlock> &interfaceBlocks)
{
    for (const sh::InterfaceBlock &interfaceBlock : interfaceBlocks)
    {
        GroupScope group(json, "Interface Block");
        json->addString("Name", interfaceBlock.name);
        json->addString("MappedName", interfaceBlock.mappedName);
        json->addString("InstanceName", interfaceBlock.instanceName);
        json->addScalar("ArraySize", interfaceBlock.arraySize);
        json->addCString("Layout", BlockLayoutTypeToString(interfaceBlock.layout));
        json->addScalar("Binding", interfaceBlock.binding);
        json->addScalar("StaticUse", interfaceBlock.staticUse);
        json->addScalar("Active", interfaceBlock.active);
        json->addCString("BlockType", BlockTypeToString(interfaceBlock.blockType));
        SerializeShaderVariablesVector(json, interfaceBlock.fields);
    }
}

void SerializeShaderState(JsonSerializer *json, const gl::ShaderState &shaderState)
{
    GroupScope group(json, "ShaderState");
    json->addString("Label", shaderState.getLabel());
    json->addCString("Type", gl::ShaderTypeToString(shaderState.getShaderType()));
    json->addScalar("Version", shaderState.getShaderVersion());
    json->addString("TranslatedSource", shaderState.getTranslatedSource());
    json->addVectorAsHash("CompiledBinary", shaderState.getCompiledBinary());
    json->addString("Source", shaderState.getSource());
    SerializeWorkGroupSize(json, shaderState.getLocalSize());
    SerializeShaderVariablesVector(json, shaderState.getInputVaryings());
    SerializeShaderVariablesVector(json, shaderState.getOutputVaryings());
    SerializeShaderVariablesVector(json, shaderState.getUniforms());
    SerializeInterfaceBlocksVector(json, shaderState.getUniformBlocks());
    SerializeInterfaceBlocksVector(json, shaderState.getShaderStorageBlocks());
    SerializeShaderVariablesVector(json, shaderState.getAllAttributes());
    SerializeShaderVariablesVector(json, shaderState.getActiveAttributes());
    SerializeShaderVariablesVector(json, shaderState.getActiveOutputVariables());
    json->addScalar("EarlyFragmentTestsOptimization",
                    shaderState.getEarlyFragmentTestsOptimization());
    json->addScalar("NumViews", shaderState.getNumViews());
    json->addScalar("SpecConstUsageBits", shaderState.getSpecConstUsageBits().bits());
    if (shaderState.getGeometryShaderInputPrimitiveType().valid())
    {
        json->addCString(
            "GeometryShaderInputPrimitiveType",
            PrimitiveModeToString(shaderState.getGeometryShaderInputPrimitiveType().value()));
    }
    if (shaderState.getGeometryShaderOutputPrimitiveType().valid())
    {
        json->addCString(
            "GeometryShaderOutputPrimitiveType",
            PrimitiveModeToString(shaderState.getGeometryShaderOutputPrimitiveType().value()));
    }
    if (shaderState.getGeometryShaderInvocations().valid())
    {
        json->addScalar("GeometryShaderInvocations",
                        shaderState.getGeometryShaderInvocations().value());
    }
    json->addCString("CompileStatus", CompileStatusToString(shaderState.getCompileStatus()));
}

void SerializeShader(JsonSerializer *json, GLuint id, gl::Shader *shader)
{
    // Ensure deterministic compilation.
    shader->resolveCompile();

    GroupScope group(json, "Shader", id);
    SerializeShaderState(json, shader->getState());
    json->addScalar("Handle", shader->getHandle().value);
    json->addScalar("RefCount", shader->getRefCount());
    json->addScalar("FlaggedForDeletion", shader->isFlaggedForDeletion());
    // Do not serialize mType because it is already serialized in SerializeShaderState.
    json->addString("InfoLogString", shader->getInfoLogString());
    // Do not serialize compiler resources string because it can vary between test modes.
    json->addScalar("CurrentMaxComputeWorkGroupInvocations",
                    shader->getCurrentMaxComputeWorkGroupInvocations());
    json->addScalar("MaxComputeSharedMemory", shader->getMaxComputeSharedMemory());
}

void SerializeVariableLocationsVector(JsonSerializer *json,
                                      const std::string &group_name,
                                      const std::vector<gl::VariableLocation> &variableLocations)
{
    GroupScope group(json, group_name);
    for (const gl::VariableLocation &variableLocation : variableLocations)
    {
        GroupScope vargroup(json, "Variable");
        json->addScalar("ArrayIndex", variableLocation.arrayIndex);
        json->addScalar("Index", variableLocation.index);
        json->addScalar("Ignored", variableLocation.ignored);
    }
}

void SerializeBlockMemberInfo(JsonSerializer *json, const sh::BlockMemberInfo &blockMemberInfo)
{
    GroupScope group(json, "BlockMemberInfo");
    json->addScalar("Offset", blockMemberInfo.offset);
    json->addScalar("Stride", blockMemberInfo.arrayStride);
    json->addScalar("MatrixStride", blockMemberInfo.matrixStride);
    json->addScalar("IsRowMajorMatrix", blockMemberInfo.isRowMajorMatrix);
    json->addScalar("TopLevelArrayStride", blockMemberInfo.topLevelArrayStride);
}

void SerializeActiveVariable(JsonSerializer *json, const gl::ActiveVariable &activeVariable)
{
    json->addScalar("ActiveShaders", activeVariable.activeShaders().to_ulong());
}

void SerializeBufferVariablesVector(JsonSerializer *json,
                                    const std::vector<gl::BufferVariable> &bufferVariables)
{
    for (const gl::BufferVariable &bufferVariable : bufferVariables)
    {
        GroupScope group(json, "BufferVariable");
        json->addScalar("BufferIndex", bufferVariable.bufferIndex);
        SerializeBlockMemberInfo(json, bufferVariable.blockInfo);
        json->addScalar("TopLevelArraySize", bufferVariable.topLevelArraySize);
        SerializeActiveVariable(json, bufferVariable);
        SerializeShaderVariable(json, bufferVariable);
    }
}

void SerializeProgramAliasedBindings(JsonSerializer *json,
                                     const gl::ProgramAliasedBindings &programAliasedBindings)
{
    for (const auto &programAliasedBinding : programAliasedBindings)
    {
        GroupScope group(json, programAliasedBinding.first);
        json->addScalar("Location", programAliasedBinding.second.location);
        json->addScalar("Aliased", programAliasedBinding.second.aliased);
    }
}

void SerializeProgramState(JsonSerializer *json, const gl::ProgramState &programState)
{
    json->addString("Label", programState.getLabel());
    SerializeWorkGroupSize(json, programState.getComputeShaderLocalSize());

    auto attachedShaders = programState.getAttachedShaders();
    std::vector<GLint> shaderHandles(attachedShaders.size());
    std::transform(attachedShaders.begin(), attachedShaders.end(), shaderHandles.begin(),
                   [](gl::Shader *shader) { return shader ? shader->getHandle().value : 0; });
    json->addVector("Handle", shaderHandles);
    json->addScalar("LocationsUsedForXfbExtension", programState.getLocationsUsedForXfbExtension());

    json->addVectorOfStrings("TransformFeedbackVaryingNames",
                             programState.getTransformFeedbackVaryingNames());
    json->addScalar("ActiveUniformBlockBindingsMask",
                    programState.getActiveUniformBlockBindingsMask().to_ulong());
    SerializeVariableLocationsVector(json, "UniformLocations", programState.getUniformLocations());
    SerializeBufferVariablesVector(json, programState.getBufferVariables());
    SerializeRange(json, programState.getAtomicCounterUniformRange());
    SerializeVariableLocationsVector(json, "SecondaryOutputLocations",
                                     programState.getSecondaryOutputLocations());
    json->addScalar("ActiveOutputVariables", programState.getActiveOutputVariables().to_ulong());
    json->addVector("OutputVariableTypes", programState.getOutputVariableTypes());
    json->addScalar("DrawBufferTypeMask", programState.getDrawBufferTypeMask().to_ulong());
    json->addScalar("BinaryRetrieveableHint", programState.hasBinaryRetrieveableHint());
    json->addScalar("Separable", programState.isSeparable());
    json->addScalar("EarlyFragmentTestsOptimization",
                    programState.hasEarlyFragmentTestsOptimization());
    json->addScalar("NumViews", programState.getNumViews());
    json->addScalar("DrawIDLocation", programState.getDrawIDLocation());
    json->addScalar("BaseVertexLocation", programState.getBaseVertexLocation());
    json->addScalar("BaseInstanceLocation", programState.getBaseInstanceLocation());
    SerializeProgramAliasedBindings(json, programState.getUniformLocationBindings());
}

void SerializeProgramBindings(JsonSerializer *json, const gl::ProgramBindings &programBindings)
{
    for (const auto &programBinding : programBindings)
    {
        json->addScalar(programBinding.first, programBinding.second);
    }
}

void SerializeProgram(JsonSerializer *json,
                      const gl::Context *context,
                      GLuint id,
                      gl::Program *program)
{
    // Ensure deterministic link.
    program->resolveLink(context);

    GroupScope group(json, "Program", id);
    SerializeProgramState(json, program->getState());
    json->addScalar("IsValidated", program->isValidated());
    SerializeProgramBindings(json, program->getAttributeBindings());
    SerializeProgramAliasedBindings(json, program->getFragmentOutputLocations());
    SerializeProgramAliasedBindings(json, program->getFragmentOutputIndexes());
    json->addScalar("IsLinked", program->isLinked());
    json->addScalar("IsFlaggedForDeletion", program->isFlaggedForDeletion());
    json->addScalar("RefCount", program->getRefCount());
    json->addScalar("ID", program->id().value);
}

void SerializeImageDesc(JsonSerializer *json, size_t descIndex, const gl::ImageDesc &imageDesc)
{
    GroupScope group(json, "ImageDesc", static_cast<int>(descIndex));
    SerializeExtents(json, imageDesc.size);
    SerializeFormat(json, imageDesc.format);
    json->addScalar("Samples", imageDesc.samples);
    json->addScalar("FixesSampleLocations", imageDesc.fixedSampleLocations);
    json->addCString("InitState", InitStateToString(imageDesc.initState));
}

void SerializeTextureState(JsonSerializer *json, const gl::TextureState &textureState)
{
    json->addCString("Type", TextureTypeToString(textureState.getType()));
    SerializeSwizzleState(json, textureState.getSwizzleState());
    {
        GroupScope samplerStateGroup(json, "SamplerState");
        SerializeSamplerState(json, textureState.getSamplerState());
    }
    json->addCString("SRGB", SrgbOverrideToString(textureState.getSRGBOverride()));
    json->addScalar("BaseLevel", textureState.getBaseLevel());
    json->addScalar("MaxLevel", textureState.getMaxLevel());
    json->addScalar("DepthStencilTextureMode", textureState.getDepthStencilTextureMode());
    json->addScalar("BeenBoundAsImage", textureState.hasBeenBoundAsImage());
    json->addScalar("ImmutableFormat", textureState.getImmutableFormat());
    json->addScalar("ImmutableLevels", textureState.getImmutableLevels());
    json->addScalar("Usage", textureState.getUsage());
    SerializeRectangle(json, "Crop", textureState.getCrop());
    json->addScalar("GenerateMipmapHint", textureState.getGenerateMipmapHint());
    json->addCString("InitState", InitStateToString(textureState.getInitState()));

    {
        GroupScope descGroup(json, "ImageDescs");
        const std::vector<gl::ImageDesc> &imageDescs = textureState.getImageDescs();
        for (size_t descIndex = 0; descIndex < imageDescs.size(); ++descIndex)
        {
            SerializeImageDesc(json, descIndex, imageDescs[descIndex]);
        }
    }
}

Result SerializeTextureData(JsonSerializer *json,
                            const gl::Context *context,
                            gl::Texture *texture,
                            ScratchBuffer *scratchBuffer)
{
    gl::ImageIndexIterator imageIter = gl::ImageIndexIterator::MakeGeneric(
        texture->getType(), 0, texture->getMipmapMaxLevel() + 1, gl::ImageIndex::kEntireLevel,
        gl::ImageIndex::kEntireLevel);
    while (imageIter.hasNext())
    {
        gl::ImageIndex index = imageIter.next();

        const gl::ImageDesc &desc = texture->getTextureState().getImageDesc(index);

        if (desc.size.empty())
            continue;

        const gl::InternalFormat &format = *desc.format.info;

        // Check for supported textures
        ASSERT(index.getType() == gl::TextureType::_2D || index.getType() == gl::TextureType::_3D ||
               index.getType() == gl::TextureType::_2DArray ||
               index.getType() == gl::TextureType::CubeMap);

        GLenum getFormat = format.format;
        GLenum getType   = format.type;

        const gl::Extents size(desc.size.width, desc.size.height, desc.size.depth);
        const gl::PixelUnpackState &unpack = context->getState().getUnpackState();

        GLuint endByte  = 0;
        bool unpackSize = format.computePackUnpackEndByte(getType, size, unpack, true, &endByte);
        ASSERT(unpackSize);
        MemoryBuffer *texelsPtr = nullptr;
        ANGLE_CHECK_GL_ALLOC(const_cast<gl::Context *>(context),
                             scratchBuffer->getInitialized(endByte, &texelsPtr, 0));

        gl::PixelPackState packState;
        packState.alignment = 1;

        std::string label = "Texels-Level" + std::to_string(index.getLevelIndex());

        if (texture->getState().getInitState() == gl::InitState::Initialized)
        {
            if (format.compressed)
            {
                // TODO: Read back compressed data. http://anglebug.com/6177
                json->addCString(label, "compressed texel data");
            }
            else
            {
                ANGLE_TRY(texture->getTexImage(context, packState, nullptr, index.getTarget(),
                                               index.getLevelIndex(), getFormat, getType,
                                               texelsPtr->data()));
                json->addBlob(label, texelsPtr->data(), texelsPtr->size());
            }
        }
        else
        {
            json->addCString(label, "not initialized");
        }
    }
    return Result::Continue;
}

Result SerializeTexture(const gl::Context *context,
                        JsonSerializer *json,
                        ScratchBuffer *scratchBuffer,
                        gl::Texture *texture)
{
    GroupScope group(json, "Texture", texture->getId());
    SerializeTextureState(json, texture->getState());
    json->addString("Label", texture->getLabel());
    // FrameCapture can not serialize mBoundSurface and mBoundStream
    // because they are likely to change with each run
    ANGLE_TRY(SerializeTextureData(json, context, texture, scratchBuffer));
    return Result::Continue;
}

void SerializeFormat(JsonSerializer *json, const angle::Format *format)
{
    json->addScalar("InternalFormat", format->glInternalFormat);
}

void SerializeVertexAttributeVector(JsonSerializer *json,
                                    const std::vector<gl::VertexAttribute> &vertexAttributes)
{
    for (const gl::VertexAttribute &vertexAttribute : vertexAttributes)
    {
        GroupScope group(json, "VertexAttribute@BindingIndex", vertexAttribute.bindingIndex);
        json->addScalar("Enabled", vertexAttribute.enabled);
        ASSERT(vertexAttribute.format);
        SerializeFormat(json, vertexAttribute.format);
        json->addScalar("RelativeOffset", vertexAttribute.relativeOffset);
        json->addScalar("VertexAttribArrayStride", vertexAttribute.vertexAttribArrayStride);
    }
}

void SerializeVertexBindingsVector(JsonSerializer *json,
                                   const std::vector<gl::VertexBinding> &vertexBindings)
{
    for (const gl::VertexBinding &vertexBinding : vertexBindings)
    {
        GroupScope group(json, "VertexBinding");
        json->addScalar("Stride", vertexBinding.getStride());
        json->addScalar("Divisor", vertexBinding.getDivisor());
        json->addScalar("Offset", vertexBinding.getOffset());
        json->addScalar("BufferID", vertexBinding.getBuffer().id().value);
        json->addScalar("BoundAttributesMask", vertexBinding.getBoundAttributesMask().to_ulong());
    }
}

void SerializeVertexArrayState(JsonSerializer *json, const gl::VertexArrayState &vertexArrayState)
{
    json->addString("Label", vertexArrayState.getLabel());
    SerializeVertexAttributeVector(json, vertexArrayState.getVertexAttributes());
    if (vertexArrayState.getElementArrayBuffer())
    {
        json->addScalar("ElementArrayBufferID",
                        vertexArrayState.getElementArrayBuffer()->id().value);
    }
    else
    {
        json->addScalar("ElementArrayBufferID", 0);
    }
    SerializeVertexBindingsVector(json, vertexArrayState.getVertexBindings());
    json->addScalar("EnabledAttributesMask",
                    vertexArrayState.getEnabledAttributesMask().to_ulong());
    json->addScalar("VertexAttributesTypeMask",
                    vertexArrayState.getVertexAttributesTypeMask().to_ulong());
    json->addScalar("ClientMemoryAttribsMask",
                    vertexArrayState.getClientMemoryAttribsMask().to_ulong());
    json->addScalar("NullPointerClientMemoryAttribsMask",
                    vertexArrayState.getNullPointerClientMemoryAttribsMask().to_ulong());
}

void SerializeVertexArray(JsonSerializer *json, gl::VertexArray *vertexArray)
{
    GroupScope group(json, "VertexArray", vertexArray->id().value);
    SerializeVertexArrayState(json, vertexArray->getState());
    json->addScalar("BufferAccessValidationEnabled",
                    vertexArray->isBufferAccessValidationEnabled());
}

}  // namespace

Result SerializeContextToString(const gl::Context *context, std::string *stringOut)
{
    JsonSerializer json;
    json.startDocument("Context");

    SerializeContextState(&json, context->getState());
    ScratchBuffer scratchBuffer(1);
    {
        const gl::FramebufferManager &framebufferManager =
            context->getState().getFramebufferManagerForCapture();
        GroupScope framebufferGroup(&json, "FramebufferManager");
        for (const auto &framebuffer : framebufferManager)
        {
            gl::Framebuffer *framebufferPtr = framebuffer.second;
            ANGLE_TRY(SerializeFramebuffer(context, &json, &scratchBuffer, framebufferPtr));
        }
    }
    {
        const gl::BufferManager &bufferManager = context->getState().getBufferManagerForCapture();
        GroupScope framebufferGroup(&json, "BufferManager");
        for (const auto &buffer : bufferManager)
        {
            gl::Buffer *bufferPtr = buffer.second;
            ANGLE_TRY(SerializeBuffer(context, &json, &scratchBuffer, bufferPtr));
        }
    }
    {
        const gl::SamplerManager &samplerManager =
            context->getState().getSamplerManagerForCapture();
        GroupScope samplerGroup(&json, "SamplerManager");
        for (const auto &sampler : samplerManager)
        {
            gl::Sampler *samplerPtr = sampler.second;
            SerializeSampler(&json, samplerPtr);
        }
    }
    {
        const gl::RenderbufferManager &renderbufferManager =
            context->getState().getRenderbufferManagerForCapture();
        GroupScope renderbufferGroup(&json, "RenderbufferManager");
        for (const auto &renderbuffer : renderbufferManager)
        {
            gl::Renderbuffer *renderbufferPtr = renderbuffer.second;
            ANGLE_TRY(SerializeRenderbuffer(context, &json, &scratchBuffer, renderbufferPtr));
        }
    }
    const gl::ShaderProgramManager &shaderProgramManager =
        context->getState().getShaderProgramManagerForCapture();
    {
        const gl::ResourceMap<gl::Shader, gl::ShaderProgramID> &shaderManager =
            shaderProgramManager.getShadersForCapture();
        GroupScope shaderGroup(&json, "ShaderManager");
        for (const auto &shader : shaderManager)
        {
            GLuint id             = shader.first;
            gl::Shader *shaderPtr = shader.second;
            SerializeShader(&json, id, shaderPtr);
        }
    }
    {
        const gl::ResourceMap<gl::Program, gl::ShaderProgramID> &programManager =
            shaderProgramManager.getProgramsForCaptureAndPerf();
        GroupScope shaderGroup(&json, "ProgramManager");
        for (const auto &program : programManager)
        {
            GLuint id               = program.first;
            gl::Program *programPtr = program.second;
            SerializeProgram(&json, context, id, programPtr);
        }
    }
    {
        const gl::TextureManager &textureManager =
            context->getState().getTextureManagerForCapture();
        GroupScope shaderGroup(&json, "TextureManager");
        for (const auto &texture : textureManager)
        {
            gl::Texture *texturePtr = texture.second;
            ANGLE_TRY(SerializeTexture(context, &json, &scratchBuffer, texturePtr));
        }
    }
    {
        const gl::VertexArrayMap &vertexArrayMap = context->getVertexArraysForCapture();
        GroupScope shaderGroup(&json, "VertexArrayMap");
        for (const auto &vertexArray : vertexArrayMap)
        {
            gl::VertexArray *vertexArrayPtr = vertexArray.second;
            SerializeVertexArray(&json, vertexArrayPtr);
        }
    }
    json.endDocument();

    *stringOut = json.data();

    scratchBuffer.clear();
    return Result::Continue;
}

}  // namespace angle
