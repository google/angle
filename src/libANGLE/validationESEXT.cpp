//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// validationESEXT.cpp: Validation functions for OpenGL ES extension entry points.

#include "libANGLE/validationESEXT_autogen.h"

#include "libANGLE/Context.h"
#include "libANGLE/ErrorStrings.h"
#include "libANGLE/validationES.h"
#include "libANGLE/validationES32.h"

namespace gl
{
using namespace err;

namespace
{
template <typename ObjectT>
bool ValidateGetImageFormatAndType(const Context *context, ObjectT *obj, GLenum format, GLenum type)
{
    GLenum implFormat = obj->getImplementationColorReadFormat(context);
    if (!ValidES3Format(format) && (format != implFormat || format == GL_NONE))
    {
        context->validationError(GL_INVALID_ENUM, kInvalidFormat);
        return false;
    }

    GLenum implType = obj->getImplementationColorReadType(context);
    if (!ValidES3Type(type) && (type != implType || type == GL_NONE))
    {
        context->validationError(GL_INVALID_ENUM, kInvalidType);
        return false;
    }

    // Format/type combinations are not yet validated.

    return true;
}

}  // namespace

bool ValidateGetTexImageANGLE(const Context *context,
                              TextureTarget target,
                              GLint level,
                              GLenum format,
                              GLenum type,
                              const void *pixels)
{
    if (!context->getExtensions().getImageANGLE)
    {
        context->validationError(GL_INVALID_OPERATION, kGetImageExtensionNotEnabled);
        return false;
    }

    if (!ValidTexture2DDestinationTarget(context, target) &&
        !ValidTexture3DDestinationTarget(context, target))
    {
        context->validationError(GL_INVALID_ENUM, kInvalidTextureTarget);
        return false;
    }

    if (level < 0)
    {
        context->validationError(GL_INVALID_VALUE, kNegativeLevel);
        return false;
    }

    TextureType textureType = TextureTargetToType(target);
    if (!ValidMipLevel(context, textureType, level))
    {
        context->validationError(GL_INVALID_VALUE, kInvalidMipLevel);
        return false;
    }

    Texture *texture = context->getTextureByTarget(target);

    if (!ValidateGetImageFormatAndType(context, texture, format, type))
    {
        return false;
    }

    GLsizei width  = static_cast<GLsizei>(texture->getWidth(target, level));
    GLsizei height = static_cast<GLsizei>(texture->getHeight(target, level));
    if (!ValidatePixelPack(context, format, type, 0, 0, width, height, -1, nullptr, pixels))
    {
        return false;
    }

    return true;
}

bool ValidateGetRenderbufferImageANGLE(const Context *context,
                                       GLenum target,
                                       GLenum format,
                                       GLenum type,
                                       const void *pixels)
{
    if (!context->getExtensions().getImageANGLE)
    {
        context->validationError(GL_INVALID_OPERATION, kGetImageExtensionNotEnabled);
        return false;
    }

    if (target != GL_RENDERBUFFER)
    {
        context->validationError(GL_INVALID_ENUM, kInvalidRenderbufferTarget);
        return false;
    }

    Renderbuffer *renderbuffer = context->getState().getCurrentRenderbuffer();

    if (!ValidateGetImageFormatAndType(context, renderbuffer, format, type))
    {
        return false;
    }

    GLsizei width  = renderbuffer->getWidth();
    GLsizei height = renderbuffer->getHeight();
    if (!ValidatePixelPack(context, format, type, 0, 0, width, height, -1, nullptr, pixels))
    {
        return false;
    }

    return true;
}

bool ValidateDrawElementsBaseVertexEXT(const Context *context,
                                       PrimitiveMode mode,
                                       GLsizei count,
                                       DrawElementsType type,
                                       const void *indices,
                                       GLint basevertex)
{
    if (!context->getExtensions().drawElementsBaseVertexAny())
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateDrawElementsCommon(context, mode, count, type, indices, 1);
}

bool ValidateDrawElementsInstancedBaseVertexEXT(const Context *context,
                                                PrimitiveMode mode,
                                                GLsizei count,
                                                DrawElementsType type,
                                                const void *indices,
                                                GLsizei instancecount,
                                                GLint basevertex)
{
    if (!context->getExtensions().drawElementsBaseVertexAny())
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateDrawElementsInstancedBase(context, mode, count, type, indices, instancecount);
}

bool ValidateDrawRangeElementsBaseVertexEXT(const Context *context,
                                            PrimitiveMode mode,
                                            GLuint start,
                                            GLuint end,
                                            GLsizei count,
                                            DrawElementsType type,
                                            const void *indices,
                                            GLint basevertex)
{
    if (!context->getExtensions().drawElementsBaseVertexAny())
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    if (end < start)
    {
        context->validationError(GL_INVALID_VALUE, kInvalidElementRange);
        return false;
    }

    if (!ValidateDrawElementsCommon(context, mode, count, type, indices, 0))
    {
        return false;
    }

    // Skip range checks for no-op calls.
    if (count <= 0)
    {
        return true;
    }

    // Note that resolving the index range is a bit slow. We should probably optimize this.
    IndexRange indexRange;
    ANGLE_VALIDATION_TRY(context->getState().getVertexArray()->getIndexRange(context, type, count,
                                                                             indices, &indexRange));

    if (indexRange.end > end || indexRange.start < start)
    {
        // GL spec says that behavior in this case is undefined - generating an error is fine.
        context->validationError(GL_INVALID_OPERATION, kExceedsElementRange);
        return false;
    }
    return true;
}

bool ValidateMultiDrawElementsBaseVertexEXT(const Context *context,
                                            PrimitiveMode mode,
                                            const GLsizei *count,
                                            DrawElementsType type,
                                            const void *const *indices,
                                            GLsizei drawcount,
                                            const GLint *basevertex)
{
    return true;
}

bool ValidateDrawElementsBaseVertexOES(const Context *context,
                                       PrimitiveMode mode,
                                       GLsizei count,
                                       DrawElementsType type,
                                       const void *indices,
                                       GLint basevertex)
{
    if (!context->getExtensions().drawElementsBaseVertexAny())
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateDrawElementsCommon(context, mode, count, type, indices, 1);
}

bool ValidateDrawElementsInstancedBaseVertexOES(const Context *context,
                                                PrimitiveMode mode,
                                                GLsizei count,
                                                DrawElementsType type,
                                                const void *indices,
                                                GLsizei instancecount,
                                                GLint basevertex)
{
    if (!context->getExtensions().drawElementsBaseVertexAny())
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateDrawElementsInstancedBase(context, mode, count, type, indices, instancecount);
}

bool ValidateDrawRangeElementsBaseVertexOES(const Context *context,
                                            PrimitiveMode mode,
                                            GLuint start,
                                            GLuint end,
                                            GLsizei count,
                                            DrawElementsType type,
                                            const void *indices,
                                            GLint basevertex)
{
    if (!context->getExtensions().drawElementsBaseVertexAny())
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    if (end < start)
    {
        context->validationError(GL_INVALID_VALUE, kInvalidElementRange);
        return false;
    }

    if (!ValidateDrawElementsCommon(context, mode, count, type, indices, 0))
    {
        return false;
    }

    // Skip range checks for no-op calls.
    if (count <= 0)
    {
        return true;
    }

    // Note that resolving the index range is a bit slow. We should probably optimize this.
    IndexRange indexRange;
    ANGLE_VALIDATION_TRY(context->getState().getVertexArray()->getIndexRange(context, type, count,
                                                                             indices, &indexRange));

    if (indexRange.end > end || indexRange.start < start)
    {
        // GL spec says that behavior in this case is undefined - generating an error is fine.
        context->validationError(GL_INVALID_OPERATION, kExceedsElementRange);
        return false;
    }
    return true;
}

bool ValidateBlendEquationSeparateiEXT(const Context *context,
                                       GLuint buf,
                                       GLenum modeRGB,
                                       GLenum modeAlpha)
{
    if (!context->getExtensions().drawBuffersIndexedEXT)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateBlendEquationSeparatei(context, buf, modeRGB, modeAlpha);
}

bool ValidateBlendEquationiEXT(const Context *context, GLuint buf, GLenum mode)
{
    if (!context->getExtensions().drawBuffersIndexedEXT)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateBlendEquationi(context, buf, mode);
}

bool ValidateBlendFuncSeparateiEXT(const Context *context,
                                   GLuint buf,
                                   GLenum srcRGB,
                                   GLenum dstRGB,
                                   GLenum srcAlpha,
                                   GLenum dstAlpha)
{
    if (!context->getExtensions().drawBuffersIndexedEXT)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateBlendFuncSeparatei(context, buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
}

bool ValidateBlendFunciEXT(const Context *context, GLuint buf, GLenum src, GLenum dst)
{
    if (!context->getExtensions().drawBuffersIndexedEXT)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateBlendFunci(context, buf, src, dst);
}

bool ValidateColorMaskiEXT(const Context *context,
                           GLuint index,
                           GLboolean r,
                           GLboolean g,
                           GLboolean b,
                           GLboolean a)
{
    if (!context->getExtensions().drawBuffersIndexedEXT)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateColorMaski(context, index, r, g, b, a);
}

bool ValidateDisableiEXT(const Context *context, GLenum target, GLuint index)
{
    if (!context->getExtensions().drawBuffersIndexedEXT)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateDisablei(context, target, index);
}

bool ValidateEnableiEXT(const Context *context, GLenum target, GLuint index)
{
    if (!context->getExtensions().drawBuffersIndexedEXT)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateEnablei(context, target, index);
}

bool ValidateIsEnablediEXT(const Context *context, GLenum target, GLuint index)
{
    if (!context->getExtensions().drawBuffersIndexedEXT)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateIsEnabledi(context, target, index);
}

bool ValidateBlendEquationSeparateiOES(const Context *context,
                                       GLuint buf,
                                       GLenum modeRGB,
                                       GLenum modeAlpha)
{
    if (!context->getExtensions().drawBuffersIndexedOES)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateBlendEquationSeparatei(context, buf, modeRGB, modeAlpha);
}

bool ValidateBlendEquationiOES(const Context *context, GLuint buf, GLenum mode)
{
    if (!context->getExtensions().drawBuffersIndexedOES)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateBlendEquationi(context, buf, mode);
}

bool ValidateBlendFuncSeparateiOES(const Context *context,
                                   GLuint buf,
                                   GLenum srcRGB,
                                   GLenum dstRGB,
                                   GLenum srcAlpha,
                                   GLenum dstAlpha)
{
    if (!context->getExtensions().drawBuffersIndexedOES)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateBlendFuncSeparatei(context, buf, srcRGB, dstRGB, srcAlpha, dstAlpha);
}

bool ValidateBlendFunciOES(const Context *context, GLuint buf, GLenum src, GLenum dst)
{
    if (!context->getExtensions().drawBuffersIndexedOES)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateBlendFunci(context, buf, src, dst);
}

bool ValidateColorMaskiOES(const Context *context,
                           GLuint index,
                           GLboolean r,
                           GLboolean g,
                           GLboolean b,
                           GLboolean a)
{
    if (!context->getExtensions().drawBuffersIndexedOES)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateColorMaski(context, index, r, g, b, a);
}

bool ValidateDisableiOES(const Context *context, GLenum target, GLuint index)
{
    if (!context->getExtensions().drawBuffersIndexedOES)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateDisablei(context, target, index);
}

bool ValidateEnableiOES(const Context *context, GLenum target, GLuint index)
{
    if (!context->getExtensions().drawBuffersIndexedOES)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateEnablei(context, target, index);
}

bool ValidateIsEnablediOES(const Context *context, GLenum target, GLuint index)
{
    if (!context->getExtensions().drawBuffersIndexedOES)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    return ValidateIsEnabledi(context, target, index);
}

bool ValidateGetInteger64vEXT(const Context *context, GLenum pname, const GLint64 *data)
{
    if (!context->getExtensions().disjointTimerQuery)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    GLenum nativeType      = GL_NONE;
    unsigned int numParams = 0;
    if (!ValidateStateQuery(context, pname, &nativeType, &numParams))
    {
        return false;
    }

    return true;
}

bool ValidateTexStorageMemFlags2DANGLE(const Context *context,
                                       TextureType targetPacked,
                                       GLsizei levels,
                                       GLenum internalFormat,
                                       GLsizei width,
                                       GLsizei height,
                                       MemoryObjectID memoryPacked,
                                       GLuint64 offset,
                                       GLbitfield createFlags,
                                       GLbitfield usageFlags)
{
    if (!context->getExtensions().memoryObjectFlagsANGLE)
    {
        context->validationError(GL_INVALID_OPERATION, kExtensionNotEnabled);
        return false;
    }

    if (!ValidateTexStorageMem2DEXT(context, targetPacked, levels, internalFormat, width, height,
                                    memoryPacked, offset))
    {
        return false;
    }

    // |createFlags| and |usageFlags| must only have bits specified by the extension.
    constexpr GLbitfield kAllCreateFlags =
        GL_CREATE_SPARSE_BINDING_BIT_ANGLE | GL_CREATE_SPARSE_RESIDENCY_BIT_ANGLE |
        GL_CREATE_SPARSE_ALIASED_BIT_ANGLE | GL_CREATE_MUTABLE_FORMAT_BIT_ANGLE |
        GL_CREATE_CUBE_COMPATIBLE_BIT_ANGLE | GL_CREATE_ALIAS_BIT_ANGLE |
        GL_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT_ANGLE | GL_CREATE_2D_ARRAY_COMPATIBLE_BIT_ANGLE |
        GL_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT_ANGLE | GL_CREATE_EXTENDED_USAGE_BIT_ANGLE |
        GL_CREATE_PROTECTED_BIT_ANGLE | GL_CREATE_DISJOINT_BIT_ANGLE |
        GL_CREATE_CORNER_SAMPLED_BIT_ANGLE | GL_CREATE_SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_ANGLE |
        GL_CREATE_SUBSAMPLED_BIT_ANGLE;

    if ((createFlags & ~kAllCreateFlags) != 0)
    {
        context->validationError(GL_INVALID_VALUE, kInvalidExternalCreateFlags);
        return false;
    }

    constexpr GLbitfield kAllUsageFlags =
        GL_USAGE_TRANSFER_SRC_BIT_ANGLE | GL_USAGE_TRANSFER_DST_BIT_ANGLE |
        GL_USAGE_SAMPLED_BIT_ANGLE | GL_USAGE_STORAGE_BIT_ANGLE |
        GL_USAGE_COLOR_ATTACHMENT_BIT_ANGLE | GL_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT_ANGLE |
        GL_USAGE_TRANSIENT_ATTACHMENT_BIT_ANGLE | GL_USAGE_INPUT_ATTACHMENT_BIT_ANGLE |
        GL_USAGE_SHADING_RATE_IMAGE_BIT_ANGLE | GL_USAGE_FRAGMENT_DENSITY_MAP_BIT_ANGLE;

    if ((usageFlags & ~kAllUsageFlags) != 0)
    {
        context->validationError(GL_INVALID_VALUE, kInvalidExternalUsageFlags);
        return false;
    }

    return true;
}

bool ValidateTexStorageMemFlags2DMultisampleANGLE(const Context *context,
                                                  TextureType targetPacked,
                                                  GLsizei samples,
                                                  GLenum internalFormat,
                                                  GLsizei width,
                                                  GLsizei height,
                                                  GLboolean fixedSampleLocations,
                                                  MemoryObjectID memoryPacked,
                                                  GLuint64 offset,
                                                  GLbitfield createFlags,
                                                  GLbitfield usageFlags)
{
    UNIMPLEMENTED();
    return false;
}

bool ValidateTexStorageMemFlags3DANGLE(const Context *context,
                                       TextureType targetPacked,
                                       GLsizei levels,
                                       GLenum internalFormat,
                                       GLsizei width,
                                       GLsizei height,
                                       GLsizei depth,
                                       MemoryObjectID memoryPacked,
                                       GLuint64 offset,
                                       GLbitfield createFlags,
                                       GLbitfield usageFlags)
{
    UNIMPLEMENTED();
    return false;
}

bool ValidateTexStorageMemFlags3DMultisampleANGLE(const Context *context,
                                                  TextureType targetPacked,
                                                  GLsizei samples,
                                                  GLenum internalFormat,
                                                  GLsizei width,
                                                  GLsizei height,
                                                  GLsizei depth,
                                                  GLboolean fixedSampleLocations,
                                                  MemoryObjectID memoryPacked,
                                                  GLuint64 offset,
                                                  GLbitfield createFlags,
                                                  GLbitfield usageFlags)
{
    UNIMPLEMENTED();
    return false;
}
}  // namespace gl
