//
// Copyright 2023 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// context_local_call_gles.cpp:
//   Helpers that set/get state that is entirely locally accessed by the context.

#include "libANGLE/context_local_call_gles_autogen.h"

#include "common/debug.h"
#include "libANGLE/queryconversions.h"

namespace gl
{
void ContextLocalClearColor(Context *context,
                            GLfloat red,
                            GLfloat green,
                            GLfloat blue,
                            GLfloat alpha)
{
    context->getMutableLocalState()->setColorClearValue(red, green, blue, alpha);
}

void ContextLocalClearDepthf(Context *context, GLfloat depth)
{
    context->getMutableLocalState()->setDepthClearValue(clamp01(depth));
}

void ContextLocalClearStencil(Context *context, GLint stencil)
{
    context->getMutableLocalState()->setStencilClearValue(stencil);
}

void ContextLocalClearColorx(Context *context,
                             GLfixed red,
                             GLfixed green,
                             GLfixed blue,
                             GLfixed alpha)
{
    context->getMutableLocalState()->setColorClearValue(
        ConvertFixedToFloat(red), ConvertFixedToFloat(green), ConvertFixedToFloat(blue),
        ConvertFixedToFloat(alpha));
}

void ContextLocalClearDepthx(Context *context, GLfixed depth)
{
    context->getMutableLocalState()->setDepthClearValue(clamp01(ConvertFixedToFloat(depth)));
}

void ContextLocalColorMask(Context *context,
                           GLboolean red,
                           GLboolean green,
                           GLboolean blue,
                           GLboolean alpha)
{
    context->getMutableLocalState()->setColorMask(ConvertToBool(red), ConvertToBool(green),
                                                  ConvertToBool(blue), ConvertToBool(alpha));
    context->onContextLocalColorMaskChange();
}

void ContextLocalColorMaski(Context *context,
                            GLuint index,
                            GLboolean r,
                            GLboolean g,
                            GLboolean b,
                            GLboolean a)
{
    context->getMutableLocalState()->setColorMaskIndexed(ConvertToBool(r), ConvertToBool(g),
                                                         ConvertToBool(b), ConvertToBool(a), index);
    context->onContextLocalColorMaskChange();
}

void ContextLocalDepthMask(Context *context, GLboolean flag)
{
    context->getMutableLocalState()->setDepthMask(ConvertToBool(flag));
}

void ContextLocalDisable(Context *context, GLenum cap)
{
    context->getMutableLocalState()->setEnableFeature(cap, false);
    context->onContextLocalCapChange();
}

void ContextLocalDisablei(Context *context, GLenum target, GLuint index)
{
    context->getMutableLocalState()->setEnableFeatureIndexed(target, false, index);
    context->onContextLocalCapChange();
}

void ContextLocalEnable(Context *context, GLenum cap)
{
    context->getMutableLocalState()->setEnableFeature(cap, true);
    context->onContextLocalCapChange();
}

void ContextLocalEnablei(Context *context, GLenum target, GLuint index)
{
    context->getMutableLocalState()->setEnableFeatureIndexed(target, true, index);
    context->onContextLocalCapChange();
}

void ContextLocalActiveTexture(Context *context, GLenum texture)
{
    context->getMutableLocalState()->setActiveSampler(texture - GL_TEXTURE0);
}

void ContextLocalCullFace(Context *context, CullFaceMode mode)
{
    context->getMutableLocalState()->setCullMode(mode);
}

void ContextLocalDepthFunc(Context *context, GLenum func)
{
    context->getMutableLocalState()->setDepthFunc(func);
}

void ContextLocalDepthRangef(Context *context, GLfloat zNear, GLfloat zFar)
{
    context->getMutableLocalState()->setDepthRange(clamp01(zNear), clamp01(zFar));
}

void ContextLocalDepthRangex(Context *context, GLfixed zNear, GLfixed zFar)
{
    context->getMutableLocalState()->setDepthRange(clamp01(ConvertFixedToFloat(zNear)),
                                                   clamp01(ConvertFixedToFloat(zFar)));
}

void ContextLocalFrontFace(Context *context, GLenum mode)
{
    context->getMutableLocalState()->setFrontFace(mode);
}

void ContextLocalLineWidth(Context *context, GLfloat width)
{
    context->getMutableLocalState()->setLineWidth(width);
}

void ContextLocalLineWidthx(Context *context, GLfixed width)
{
    context->getMutableLocalState()->setLineWidth(ConvertFixedToFloat(width));
}

void ContextLocalPolygonOffset(Context *context, GLfloat factor, GLfloat units)
{
    context->getMutableLocalState()->setPolygonOffsetParams(factor, units, 0.0f);
}

void ContextLocalPolygonOffsetClamp(Context *context, GLfloat factor, GLfloat units, GLfloat clamp)
{
    context->getMutableLocalState()->setPolygonOffsetParams(factor, units, clamp);
}

void ContextLocalPolygonOffsetx(Context *context, GLfixed factor, GLfixed units)
{
    context->getMutableLocalState()->setPolygonOffsetParams(ConvertFixedToFloat(factor),
                                                            ConvertFixedToFloat(units), 0.0f);
}

void ContextLocalSampleCoverage(Context *context, GLfloat value, GLboolean invert)
{
    context->getMutableLocalState()->setSampleCoverageParams(clamp01(value), ConvertToBool(invert));
}

void ContextLocalSampleCoveragex(Context *context, GLclampx value, GLboolean invert)
{
    const GLclampf valuef = ConvertFixedToFloat(value);
    context->getMutableLocalState()->setSampleCoverageParams(clamp01(valuef),
                                                             ConvertToBool(invert));
}

void ContextLocalScissor(Context *context, GLint x, GLint y, GLsizei width, GLsizei height)
{
    context->getMutableLocalState()->setScissorParams(x, y, width, height);
}

void ContextLocalVertexAttrib1f(Context *context, GLuint index, GLfloat x)
{
    GLfloat vals[4] = {x, 0, 0, 1};
    context->getMutableLocalState()->setVertexAttribf(index, vals);
    context->onContextLocalDefaultVertexAttributeChange();
}

void ContextLocalVertexAttrib1fv(Context *context, GLuint index, const GLfloat *values)
{
    GLfloat vals[4] = {values[0], 0, 0, 1};
    context->getMutableLocalState()->setVertexAttribf(index, vals);
    context->onContextLocalDefaultVertexAttributeChange();
}

void ContextLocalVertexAttrib2f(Context *context, GLuint index, GLfloat x, GLfloat y)
{
    GLfloat vals[4] = {x, y, 0, 1};
    context->getMutableLocalState()->setVertexAttribf(index, vals);
    context->onContextLocalDefaultVertexAttributeChange();
}

void ContextLocalVertexAttrib2fv(Context *context, GLuint index, const GLfloat *values)
{
    GLfloat vals[4] = {values[0], values[1], 0, 1};
    context->getMutableLocalState()->setVertexAttribf(index, vals);
    context->onContextLocalDefaultVertexAttributeChange();
}

void ContextLocalVertexAttrib3f(Context *context, GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
    GLfloat vals[4] = {x, y, z, 1};
    context->getMutableLocalState()->setVertexAttribf(index, vals);
    context->onContextLocalDefaultVertexAttributeChange();
}

void ContextLocalVertexAttrib3fv(Context *context, GLuint index, const GLfloat *values)
{
    GLfloat vals[4] = {values[0], values[1], values[2], 1};
    context->getMutableLocalState()->setVertexAttribf(index, vals);
    context->onContextLocalDefaultVertexAttributeChange();
}

void ContextLocalVertexAttrib4f(Context *context,
                                GLuint index,
                                GLfloat x,
                                GLfloat y,
                                GLfloat z,
                                GLfloat w)
{
    GLfloat vals[4] = {x, y, z, w};
    context->getMutableLocalState()->setVertexAttribf(index, vals);
    context->onContextLocalDefaultVertexAttributeChange();
}

void ContextLocalVertexAttrib4fv(Context *context, GLuint index, const GLfloat *values)
{
    context->getMutableLocalState()->setVertexAttribf(index, values);
    context->onContextLocalDefaultVertexAttributeChange();
}

void ContextLocalVertexAttribI4i(Context *context, GLuint index, GLint x, GLint y, GLint z, GLint w)
{
    GLint vals[4] = {x, y, z, w};
    context->getMutableLocalState()->setVertexAttribi(index, vals);
    context->onContextLocalDefaultVertexAttributeChange();
}

void ContextLocalVertexAttribI4iv(Context *context, GLuint index, const GLint *values)
{
    context->getMutableLocalState()->setVertexAttribi(index, values);
    context->onContextLocalDefaultVertexAttributeChange();
}

void ContextLocalVertexAttribI4ui(Context *context,
                                  GLuint index,
                                  GLuint x,
                                  GLuint y,
                                  GLuint z,
                                  GLuint w)
{
    GLuint vals[4] = {x, y, z, w};
    context->getMutableLocalState()->setVertexAttribu(index, vals);
    context->onContextLocalDefaultVertexAttributeChange();
}

void ContextLocalVertexAttribI4uiv(Context *context, GLuint index, const GLuint *values)
{
    context->getMutableLocalState()->setVertexAttribu(index, values);
    context->onContextLocalDefaultVertexAttributeChange();
}

void ContextLocalViewport(Context *context, GLint x, GLint y, GLsizei width, GLsizei height)
{
    context->getMutableLocalState()->setViewportParams(x, y, width, height);
}

void ContextLocalSampleMaski(Context *context, GLuint maskNumber, GLbitfield mask)
{
    context->getMutableLocalState()->setSampleMaskParams(maskNumber, mask);
}

void ContextLocalMinSampleShading(Context *context, GLfloat value)
{
    context->getMutableLocalState()->setMinSampleShading(value);
}

void ContextLocalPrimitiveBoundingBox(Context *context,
                                      GLfloat minX,
                                      GLfloat minY,
                                      GLfloat minZ,
                                      GLfloat minW,
                                      GLfloat maxX,
                                      GLfloat maxY,
                                      GLfloat maxZ,
                                      GLfloat maxW)
{
    context->getMutableLocalState()->setBoundingBox(minX, minY, minZ, minW, maxX, maxY, maxZ, maxW);
}

void ContextLocalLogicOp(Context *context, LogicalOperation opcode)
{
    context->getMutableGLES1State()->setLogicOp(opcode);
}

void ContextLocalLogicOpANGLE(Context *context, LogicalOperation opcode)
{
    context->getMutableLocalState()->setLogicOp(opcode);
}

void ContextLocalPolygonMode(Context *context, GLenum face, PolygonMode mode)
{
    ASSERT(face == GL_FRONT_AND_BACK);
    context->getMutableLocalState()->setPolygonMode(mode);
}

void ContextLocalPolygonModeNV(Context *context, GLenum face, PolygonMode mode)
{
    ContextLocalPolygonMode(context, face, mode);
}

void ContextLocalProvokingVertex(Context *context, ProvokingVertexConvention provokeMode)
{
    context->getMutableLocalState()->setProvokingVertex(provokeMode);
}

void ContextLocalCoverageModulation(Context *context, GLenum components)
{
    context->getMutableLocalState()->setCoverageModulation(components);
}

void ContextLocalClipControl(Context *context, ClipOrigin origin, ClipDepthMode depth)
{
    context->getMutableLocalState()->setClipControl(origin, depth);
}

void ContextLocalShadingRate(Context *context, GLenum rate)
{
    context->getMutableLocalState()->setShadingRate(rate);
}

void ContextLocalBlendColor(Context *context,
                            GLfloat red,
                            GLfloat green,
                            GLfloat blue,
                            GLfloat alpha)
{
    context->getMutableLocalState()->setBlendColor(red, green, blue, alpha);
}

void ContextLocalBlendEquation(Context *context, GLenum mode)
{
    context->getMutableLocalState()->setBlendEquation(mode, mode);
    context->onContextLocalBlendEquationChange();
}

void ContextLocalBlendEquationi(Context *context, GLuint buf, GLenum mode)
{
    context->getMutableLocalState()->setBlendEquationIndexed(mode, mode, buf);
    context->onContextLocalBlendEquationChange();
}

void ContextLocalBlendEquationSeparate(Context *context, GLenum modeRGB, GLenum modeAlpha)
{
    context->getMutableLocalState()->setBlendEquation(modeRGB, modeAlpha);
    context->onContextLocalBlendEquationChange();
}

void ContextLocalBlendEquationSeparatei(Context *context,
                                        GLuint buf,
                                        GLenum modeRGB,
                                        GLenum modeAlpha)
{
    context->getMutableLocalState()->setBlendEquationIndexed(modeRGB, modeAlpha, buf);
    context->onContextLocalBlendEquationChange();
}

void ContextLocalBlendFunc(Context *context, GLenum sfactor, GLenum dfactor)
{
    context->getMutableLocalState()->setBlendFactors(sfactor, dfactor, sfactor, dfactor);
}

void ContextLocalBlendFunci(Context *context, GLuint buf, GLenum src, GLenum dst)
{
    context->getMutableLocalState()->setBlendFactorsIndexed(src, dst, src, dst, buf);
    if (context->getState().noSimultaneousConstantColorAndAlphaBlendFunc())
    {
        context->onContextLocalBlendFuncIndexedChange();
    }
}

void ContextLocalBlendFuncSeparate(Context *context,
                                   GLenum srcRGB,
                                   GLenum dstRGB,
                                   GLenum srcAlpha,
                                   GLenum dstAlpha)
{
    context->getMutableLocalState()->setBlendFactors(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void ContextLocalBlendFuncSeparatei(Context *context,
                                    GLuint buf,
                                    GLenum srcRGB,
                                    GLenum dstRGB,
                                    GLenum srcAlpha,
                                    GLenum dstAlpha)
{
    context->getMutableLocalState()->setBlendFactorsIndexed(srcRGB, dstRGB, srcAlpha, dstAlpha,
                                                            buf);
    if (context->getState().noSimultaneousConstantColorAndAlphaBlendFunc())
    {
        context->onContextLocalBlendFuncIndexedChange();
    }
}

void ContextLocalStencilFunc(Context *context, GLenum func, GLint ref, GLuint mask)
{
    ContextLocalStencilFuncSeparate(context, GL_FRONT_AND_BACK, func, ref, mask);
}

void ContextLocalStencilFuncSeparate(Context *context,
                                     GLenum face,
                                     GLenum func,
                                     GLint ref,
                                     GLuint mask)
{
    GLint clampedRef = gl::clamp(ref, 0, std::numeric_limits<uint8_t>::max());
    if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
    {
        context->getMutableLocalState()->setStencilParams(func, clampedRef, mask);
    }

    if (face == GL_BACK || face == GL_FRONT_AND_BACK)
    {
        context->getMutableLocalState()->setStencilBackParams(func, clampedRef, mask);
    }

    context->onContextLocalStencilStateChange();
}

void ContextLocalStencilMask(Context *context, GLuint mask)
{
    ContextLocalStencilMaskSeparate(context, GL_FRONT_AND_BACK, mask);
}

void ContextLocalStencilMaskSeparate(Context *context, GLenum face, GLuint mask)
{
    if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
    {
        context->getMutableLocalState()->setStencilWritemask(mask);
    }

    if (face == GL_BACK || face == GL_FRONT_AND_BACK)
    {
        context->getMutableLocalState()->setStencilBackWritemask(mask);
    }

    context->onContextLocalStencilStateChange();
}

void ContextLocalStencilOp(Context *context, GLenum fail, GLenum zfail, GLenum zpass)
{
    ContextLocalStencilOpSeparate(context, GL_FRONT_AND_BACK, fail, zfail, zpass);
}

void ContextLocalStencilOpSeparate(Context *context,
                                   GLenum face,
                                   GLenum fail,
                                   GLenum zfail,
                                   GLenum zpass)
{
    if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
    {
        context->getMutableLocalState()->setStencilOperations(fail, zfail, zpass);
    }

    if (face == GL_BACK || face == GL_FRONT_AND_BACK)
    {
        context->getMutableLocalState()->setStencilBackOperations(fail, zfail, zpass);
    }
}

void ContextLocalPixelStorei(Context *context, GLenum pname, GLint param)
{
    switch (pname)
    {
        case GL_UNPACK_ALIGNMENT:
            context->getMutableLocalState()->setUnpackAlignment(param);
            break;

        case GL_PACK_ALIGNMENT:
            context->getMutableLocalState()->setPackAlignment(param);
            break;

        case GL_PACK_REVERSE_ROW_ORDER_ANGLE:
            context->getMutableLocalState()->setPackReverseRowOrder(param != 0);
            break;

        case GL_UNPACK_ROW_LENGTH:
            ASSERT((context->getClientMajorVersion() >= 3) ||
                   context->getExtensions().unpackSubimageEXT);
            context->getMutableLocalState()->setUnpackRowLength(param);
            break;

        case GL_UNPACK_IMAGE_HEIGHT:
            ASSERT(context->getClientMajorVersion() >= 3);
            context->getMutableLocalState()->setUnpackImageHeight(param);
            break;

        case GL_UNPACK_SKIP_IMAGES:
            ASSERT(context->getClientMajorVersion() >= 3);
            context->getMutableLocalState()->setUnpackSkipImages(param);
            break;

        case GL_UNPACK_SKIP_ROWS:
            ASSERT((context->getClientMajorVersion() >= 3) ||
                   context->getExtensions().unpackSubimageEXT);
            context->getMutableLocalState()->setUnpackSkipRows(param);
            break;

        case GL_UNPACK_SKIP_PIXELS:
            ASSERT((context->getClientMajorVersion() >= 3) ||
                   context->getExtensions().unpackSubimageEXT);
            context->getMutableLocalState()->setUnpackSkipPixels(param);
            break;

        case GL_PACK_ROW_LENGTH:
            ASSERT((context->getClientMajorVersion() >= 3) ||
                   context->getExtensions().packSubimageNV);
            context->getMutableLocalState()->setPackRowLength(param);
            break;

        case GL_PACK_SKIP_ROWS:
            ASSERT((context->getClientMajorVersion() >= 3) ||
                   context->getExtensions().packSubimageNV);
            context->getMutableLocalState()->setPackSkipRows(param);
            break;

        case GL_PACK_SKIP_PIXELS:
            ASSERT((context->getClientMajorVersion() >= 3) ||
                   context->getExtensions().packSubimageNV);
            context->getMutableLocalState()->setPackSkipPixels(param);
            break;

        default:
            UNREACHABLE();
            return;
    }
}

void ContextLocalHint(Context *context, GLenum target, GLenum mode)
{
    switch (target)
    {
        case GL_GENERATE_MIPMAP_HINT:
            context->getMutableLocalState()->setGenerateMipmapHint(mode);
            break;

        case GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES:
            context->getMutableLocalState()->setFragmentShaderDerivativeHint(mode);
            break;

        case GL_PERSPECTIVE_CORRECTION_HINT:
        case GL_POINT_SMOOTH_HINT:
        case GL_LINE_SMOOTH_HINT:
        case GL_FOG_HINT:
            context->getMutableGLES1State()->setHint(target, mode);
            break;
        case GL_TEXTURE_FILTERING_HINT_CHROMIUM:
            context->getMutableLocalState()->setTextureFilteringHint(mode);
            break;
        default:
            UNREACHABLE();
            return;
    }
}

GLboolean ContextLocalIsEnabled(Context *context, GLenum cap)
{
    return context->getState().localState().getEnableFeature(cap);
}

GLboolean ContextLocalIsEnabledi(Context *context, GLenum target, GLuint index)
{
    return context->getState().localState().getEnableFeatureIndexed(target, index);
}

void ContextLocalPatchParameteri(Context *context, GLenum pname, GLint value)
{
    switch (pname)
    {
        case GL_PATCH_VERTICES:
            context->getMutableLocalState()->setPatchVertices(value);
            break;
        default:
            break;
    }
}
}  // namespace gl
