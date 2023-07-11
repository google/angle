//
// Copyright 2023 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// context_private_call_gles.cpp:
//   Helpers that set/get state that is entirely locally accessed by the context.

#include "libANGLE/context_private_call_gles_autogen.h"

#include "common/debug.h"
#include "libANGLE/queryconversions.h"
#include "libANGLE/queryutils.h"

namespace
{
angle::Mat4 FixedMatrixToMat4(const GLfixed *m)
{
    angle::Mat4 matrixAsFloat;
    GLfloat *floatData = matrixAsFloat.data();

    for (int i = 0; i < 16; i++)
    {
        floatData[i] = gl::ConvertFixedToFloat(m[i]);
    }

    return matrixAsFloat;
}
}  // namespace

namespace gl
{
void ContextPrivateClearColor(Context *context,
                              GLfloat red,
                              GLfloat green,
                              GLfloat blue,
                              GLfloat alpha)
{
    context->getMutablePrivateState()->setColorClearValue(red, green, blue, alpha);
}

void ContextPrivateClearDepthf(Context *context, GLfloat depth)
{
    context->getMutablePrivateState()->setDepthClearValue(clamp01(depth));
}

void ContextPrivateClearStencil(Context *context, GLint stencil)
{
    context->getMutablePrivateState()->setStencilClearValue(stencil);
}

void ContextPrivateClearColorx(Context *context,
                               GLfixed red,
                               GLfixed green,
                               GLfixed blue,
                               GLfixed alpha)
{
    ContextPrivateClearColor(context, ConvertFixedToFloat(red), ConvertFixedToFloat(green),
                             ConvertFixedToFloat(blue), ConvertFixedToFloat(alpha));
}

void ContextPrivateClearDepthx(Context *context, GLfixed depth)
{
    ContextPrivateClearDepthf(context, ConvertFixedToFloat(depth));
}

void ContextPrivateColorMask(Context *context,
                             GLboolean red,
                             GLboolean green,
                             GLboolean blue,
                             GLboolean alpha)
{
    context->getMutablePrivateState()->setColorMask(ConvertToBool(red), ConvertToBool(green),
                                                    ConvertToBool(blue), ConvertToBool(alpha));
    context->onContextPrivateColorMaskChange();
}

void ContextPrivateColorMaski(Context *context,
                              GLuint index,
                              GLboolean r,
                              GLboolean g,
                              GLboolean b,
                              GLboolean a)
{
    context->getMutablePrivateState()->setColorMaskIndexed(
        ConvertToBool(r), ConvertToBool(g), ConvertToBool(b), ConvertToBool(a), index);
    context->onContextPrivateColorMaskChange();
}

void ContextPrivateDepthMask(Context *context, GLboolean flag)
{
    context->getMutablePrivateState()->setDepthMask(ConvertToBool(flag));
}

void ContextPrivateDisable(Context *context, GLenum cap)
{
    context->getMutablePrivateState()->setEnableFeature(cap, false);
    context->onContextPrivateCapChange();
}

void ContextPrivateDisablei(Context *context, GLenum target, GLuint index)
{
    context->getMutablePrivateState()->setEnableFeatureIndexed(target, false, index);
    context->onContextPrivateCapChange();
}

void ContextPrivateEnable(Context *context, GLenum cap)
{
    context->getMutablePrivateState()->setEnableFeature(cap, true);
    context->onContextPrivateCapChange();
}

void ContextPrivateEnablei(Context *context, GLenum target, GLuint index)
{
    context->getMutablePrivateState()->setEnableFeatureIndexed(target, true, index);
    context->onContextPrivateCapChange();
}

void ContextPrivateActiveTexture(Context *context, GLenum texture)
{
    context->getMutablePrivateState()->setActiveSampler(texture - GL_TEXTURE0);
}

void ContextPrivateCullFace(Context *context, CullFaceMode mode)
{
    context->getMutablePrivateState()->setCullMode(mode);
}

void ContextPrivateDepthFunc(Context *context, GLenum func)
{
    context->getMutablePrivateState()->setDepthFunc(func);
}

void ContextPrivateDepthRangef(Context *context, GLfloat zNear, GLfloat zFar)
{
    context->getMutablePrivateState()->setDepthRange(clamp01(zNear), clamp01(zFar));
}

void ContextPrivateDepthRangex(Context *context, GLfixed zNear, GLfixed zFar)
{
    ContextPrivateDepthRangef(context, ConvertFixedToFloat(zNear), ConvertFixedToFloat(zFar));
}

void ContextPrivateFrontFace(Context *context, GLenum mode)
{
    context->getMutablePrivateState()->setFrontFace(mode);
}

void ContextPrivateLineWidth(Context *context, GLfloat width)
{
    context->getMutablePrivateState()->setLineWidth(width);
}

void ContextPrivateLineWidthx(Context *context, GLfixed width)
{
    ContextPrivateLineWidth(context, ConvertFixedToFloat(width));
}

void ContextPrivatePolygonOffset(Context *context, GLfloat factor, GLfloat units)
{
    context->getMutablePrivateState()->setPolygonOffsetParams(factor, units, 0.0f);
}

void ContextPrivatePolygonOffsetClamp(Context *context,
                                      GLfloat factor,
                                      GLfloat units,
                                      GLfloat clamp)
{
    context->getMutablePrivateState()->setPolygonOffsetParams(factor, units, clamp);
}

void ContextPrivatePolygonOffsetx(Context *context, GLfixed factor, GLfixed units)
{
    ContextPrivatePolygonOffsetClamp(context, ConvertFixedToFloat(factor),
                                     ConvertFixedToFloat(units), 0.0f);
}

void ContextPrivateSampleCoverage(Context *context, GLfloat value, GLboolean invert)
{
    context->getMutablePrivateState()->setSampleCoverageParams(clamp01(value),
                                                               ConvertToBool(invert));
}

void ContextPrivateSampleCoveragex(Context *context, GLclampx value, GLboolean invert)
{
    ContextPrivateSampleCoverage(context, ConvertFixedToFloat(value), invert);
}

void ContextPrivateScissor(Context *context, GLint x, GLint y, GLsizei width, GLsizei height)
{
    context->getMutablePrivateState()->setScissorParams(x, y, width, height);
}

void ContextPrivateVertexAttrib1f(Context *context, GLuint index, GLfloat x)
{
    GLfloat vals[4] = {x, 0, 0, 1};
    context->getMutablePrivateState()->setVertexAttribf(index, vals);
    context->onContextPrivateDefaultVertexAttributeChange();
}

void ContextPrivateVertexAttrib1fv(Context *context, GLuint index, const GLfloat *values)
{
    GLfloat vals[4] = {values[0], 0, 0, 1};
    context->getMutablePrivateState()->setVertexAttribf(index, vals);
    context->onContextPrivateDefaultVertexAttributeChange();
}

void ContextPrivateVertexAttrib2f(Context *context, GLuint index, GLfloat x, GLfloat y)
{
    GLfloat vals[4] = {x, y, 0, 1};
    context->getMutablePrivateState()->setVertexAttribf(index, vals);
    context->onContextPrivateDefaultVertexAttributeChange();
}

void ContextPrivateVertexAttrib2fv(Context *context, GLuint index, const GLfloat *values)
{
    GLfloat vals[4] = {values[0], values[1], 0, 1};
    context->getMutablePrivateState()->setVertexAttribf(index, vals);
    context->onContextPrivateDefaultVertexAttributeChange();
}

void ContextPrivateVertexAttrib3f(Context *context, GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
    GLfloat vals[4] = {x, y, z, 1};
    context->getMutablePrivateState()->setVertexAttribf(index, vals);
    context->onContextPrivateDefaultVertexAttributeChange();
}

void ContextPrivateVertexAttrib3fv(Context *context, GLuint index, const GLfloat *values)
{
    GLfloat vals[4] = {values[0], values[1], values[2], 1};
    context->getMutablePrivateState()->setVertexAttribf(index, vals);
    context->onContextPrivateDefaultVertexAttributeChange();
}

void ContextPrivateVertexAttrib4f(Context *context,
                                  GLuint index,
                                  GLfloat x,
                                  GLfloat y,
                                  GLfloat z,
                                  GLfloat w)
{
    GLfloat vals[4] = {x, y, z, w};
    context->getMutablePrivateState()->setVertexAttribf(index, vals);
    context->onContextPrivateDefaultVertexAttributeChange();
}

void ContextPrivateVertexAttrib4fv(Context *context, GLuint index, const GLfloat *values)
{
    context->getMutablePrivateState()->setVertexAttribf(index, values);
    context->onContextPrivateDefaultVertexAttributeChange();
}

void ContextPrivateVertexAttribI4i(Context *context,
                                   GLuint index,
                                   GLint x,
                                   GLint y,
                                   GLint z,
                                   GLint w)
{
    GLint vals[4] = {x, y, z, w};
    context->getMutablePrivateState()->setVertexAttribi(index, vals);
    context->onContextPrivateDefaultVertexAttributeChange();
}

void ContextPrivateVertexAttribI4iv(Context *context, GLuint index, const GLint *values)
{
    context->getMutablePrivateState()->setVertexAttribi(index, values);
    context->onContextPrivateDefaultVertexAttributeChange();
}

void ContextPrivateVertexAttribI4ui(Context *context,
                                    GLuint index,
                                    GLuint x,
                                    GLuint y,
                                    GLuint z,
                                    GLuint w)
{
    GLuint vals[4] = {x, y, z, w};
    context->getMutablePrivateState()->setVertexAttribu(index, vals);
    context->onContextPrivateDefaultVertexAttributeChange();
}

void ContextPrivateVertexAttribI4uiv(Context *context, GLuint index, const GLuint *values)
{
    context->getMutablePrivateState()->setVertexAttribu(index, values);
    context->onContextPrivateDefaultVertexAttributeChange();
}

void ContextPrivateViewport(Context *context, GLint x, GLint y, GLsizei width, GLsizei height)
{
    context->getMutablePrivateState()->setViewportParams(x, y, width, height);
}

void ContextPrivateSampleMaski(Context *context, GLuint maskNumber, GLbitfield mask)
{
    context->getMutablePrivateState()->setSampleMaskParams(maskNumber, mask);
}

void ContextPrivateMinSampleShading(Context *context, GLfloat value)
{
    context->getMutablePrivateState()->setMinSampleShading(value);
}

void ContextPrivatePrimitiveBoundingBox(Context *context,
                                        GLfloat minX,
                                        GLfloat minY,
                                        GLfloat minZ,
                                        GLfloat minW,
                                        GLfloat maxX,
                                        GLfloat maxY,
                                        GLfloat maxZ,
                                        GLfloat maxW)
{
    context->getMutablePrivateState()->setBoundingBox(minX, minY, minZ, minW, maxX, maxY, maxZ,
                                                      maxW);
}

void ContextPrivateLogicOp(Context *context, LogicalOperation opcode)
{
    context->getMutableGLES1State()->setLogicOp(opcode);
}

void ContextPrivateLogicOpANGLE(Context *context, LogicalOperation opcode)
{
    context->getMutablePrivateState()->setLogicOp(opcode);
}

void ContextPrivatePolygonMode(Context *context, GLenum face, PolygonMode mode)
{
    ASSERT(face == GL_FRONT_AND_BACK);
    context->getMutablePrivateState()->setPolygonMode(mode);
}

void ContextPrivatePolygonModeNV(Context *context, GLenum face, PolygonMode mode)
{
    ContextPrivatePolygonMode(context, face, mode);
}

void ContextPrivateProvokingVertex(Context *context, ProvokingVertexConvention provokeMode)
{
    context->getMutablePrivateState()->setProvokingVertex(provokeMode);
}

void ContextPrivateCoverageModulation(Context *context, GLenum components)
{
    context->getMutablePrivateState()->setCoverageModulation(components);
}

void ContextPrivateClipControl(Context *context, ClipOrigin origin, ClipDepthMode depth)
{
    context->getMutablePrivateState()->setClipControl(origin, depth);
}

void ContextPrivateShadingRate(Context *context, GLenum rate)
{
    context->getMutablePrivateState()->setShadingRate(rate);
}

void ContextPrivateBlendColor(Context *context,
                              GLfloat red,
                              GLfloat green,
                              GLfloat blue,
                              GLfloat alpha)
{
    context->getMutablePrivateState()->setBlendColor(red, green, blue, alpha);
}

void ContextPrivateBlendEquation(Context *context, GLenum mode)
{
    context->getMutablePrivateState()->setBlendEquation(mode, mode);
    context->onContextPrivateBlendEquationChange();
}

void ContextPrivateBlendEquationi(Context *context, GLuint buf, GLenum mode)
{
    context->getMutablePrivateState()->setBlendEquationIndexed(mode, mode, buf);
    context->onContextPrivateBlendEquationChange();
}

void ContextPrivateBlendEquationSeparate(Context *context, GLenum modeRGB, GLenum modeAlpha)
{
    context->getMutablePrivateState()->setBlendEquation(modeRGB, modeAlpha);
    context->onContextPrivateBlendEquationChange();
}

void ContextPrivateBlendEquationSeparatei(Context *context,
                                          GLuint buf,
                                          GLenum modeRGB,
                                          GLenum modeAlpha)
{
    context->getMutablePrivateState()->setBlendEquationIndexed(modeRGB, modeAlpha, buf);
    context->onContextPrivateBlendEquationChange();
}

void ContextPrivateBlendFunc(Context *context, GLenum sfactor, GLenum dfactor)
{
    context->getMutablePrivateState()->setBlendFactors(sfactor, dfactor, sfactor, dfactor);
}

void ContextPrivateBlendFunci(Context *context, GLuint buf, GLenum src, GLenum dst)
{
    context->getMutablePrivateState()->setBlendFactorsIndexed(src, dst, src, dst, buf);
    if (context->getState().noSimultaneousConstantColorAndAlphaBlendFunc())
    {
        context->onContextPrivateBlendFuncIndexedChange();
    }
}

void ContextPrivateBlendFuncSeparate(Context *context,
                                     GLenum srcRGB,
                                     GLenum dstRGB,
                                     GLenum srcAlpha,
                                     GLenum dstAlpha)
{
    context->getMutablePrivateState()->setBlendFactors(srcRGB, dstRGB, srcAlpha, dstAlpha);
}

void ContextPrivateBlendFuncSeparatei(Context *context,
                                      GLuint buf,
                                      GLenum srcRGB,
                                      GLenum dstRGB,
                                      GLenum srcAlpha,
                                      GLenum dstAlpha)
{
    context->getMutablePrivateState()->setBlendFactorsIndexed(srcRGB, dstRGB, srcAlpha, dstAlpha,
                                                              buf);
    if (context->getState().noSimultaneousConstantColorAndAlphaBlendFunc())
    {
        context->onContextPrivateBlendFuncIndexedChange();
    }
}

void ContextPrivateStencilFunc(Context *context, GLenum func, GLint ref, GLuint mask)
{
    ContextPrivateStencilFuncSeparate(context, GL_FRONT_AND_BACK, func, ref, mask);
}

void ContextPrivateStencilFuncSeparate(Context *context,
                                       GLenum face,
                                       GLenum func,
                                       GLint ref,
                                       GLuint mask)
{
    GLint clampedRef = gl::clamp(ref, 0, std::numeric_limits<uint8_t>::max());
    if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
    {
        context->getMutablePrivateState()->setStencilParams(func, clampedRef, mask);
    }

    if (face == GL_BACK || face == GL_FRONT_AND_BACK)
    {
        context->getMutablePrivateState()->setStencilBackParams(func, clampedRef, mask);
    }

    context->onContextPrivateStencilStateChange();
}

void ContextPrivateStencilMask(Context *context, GLuint mask)
{
    ContextPrivateStencilMaskSeparate(context, GL_FRONT_AND_BACK, mask);
}

void ContextPrivateStencilMaskSeparate(Context *context, GLenum face, GLuint mask)
{
    if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
    {
        context->getMutablePrivateState()->setStencilWritemask(mask);
    }

    if (face == GL_BACK || face == GL_FRONT_AND_BACK)
    {
        context->getMutablePrivateState()->setStencilBackWritemask(mask);
    }

    context->onContextPrivateStencilStateChange();
}

void ContextPrivateStencilOp(Context *context, GLenum fail, GLenum zfail, GLenum zpass)
{
    ContextPrivateStencilOpSeparate(context, GL_FRONT_AND_BACK, fail, zfail, zpass);
}

void ContextPrivateStencilOpSeparate(Context *context,
                                     GLenum face,
                                     GLenum fail,
                                     GLenum zfail,
                                     GLenum zpass)
{
    if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
    {
        context->getMutablePrivateState()->setStencilOperations(fail, zfail, zpass);
    }

    if (face == GL_BACK || face == GL_FRONT_AND_BACK)
    {
        context->getMutablePrivateState()->setStencilBackOperations(fail, zfail, zpass);
    }
}

void ContextPrivatePixelStorei(Context *context, GLenum pname, GLint param)
{
    switch (pname)
    {
        case GL_UNPACK_ALIGNMENT:
            context->getMutablePrivateState()->setUnpackAlignment(param);
            break;

        case GL_PACK_ALIGNMENT:
            context->getMutablePrivateState()->setPackAlignment(param);
            break;

        case GL_PACK_REVERSE_ROW_ORDER_ANGLE:
            context->getMutablePrivateState()->setPackReverseRowOrder(param != 0);
            break;

        case GL_UNPACK_ROW_LENGTH:
            ASSERT((context->getClientMajorVersion() >= 3) ||
                   context->getExtensions().unpackSubimageEXT);
            context->getMutablePrivateState()->setUnpackRowLength(param);
            break;

        case GL_UNPACK_IMAGE_HEIGHT:
            ASSERT(context->getClientMajorVersion() >= 3);
            context->getMutablePrivateState()->setUnpackImageHeight(param);
            break;

        case GL_UNPACK_SKIP_IMAGES:
            ASSERT(context->getClientMajorVersion() >= 3);
            context->getMutablePrivateState()->setUnpackSkipImages(param);
            break;

        case GL_UNPACK_SKIP_ROWS:
            ASSERT((context->getClientMajorVersion() >= 3) ||
                   context->getExtensions().unpackSubimageEXT);
            context->getMutablePrivateState()->setUnpackSkipRows(param);
            break;

        case GL_UNPACK_SKIP_PIXELS:
            ASSERT((context->getClientMajorVersion() >= 3) ||
                   context->getExtensions().unpackSubimageEXT);
            context->getMutablePrivateState()->setUnpackSkipPixels(param);
            break;

        case GL_PACK_ROW_LENGTH:
            ASSERT((context->getClientMajorVersion() >= 3) ||
                   context->getExtensions().packSubimageNV);
            context->getMutablePrivateState()->setPackRowLength(param);
            break;

        case GL_PACK_SKIP_ROWS:
            ASSERT((context->getClientMajorVersion() >= 3) ||
                   context->getExtensions().packSubimageNV);
            context->getMutablePrivateState()->setPackSkipRows(param);
            break;

        case GL_PACK_SKIP_PIXELS:
            ASSERT((context->getClientMajorVersion() >= 3) ||
                   context->getExtensions().packSubimageNV);
            context->getMutablePrivateState()->setPackSkipPixels(param);
            break;

        default:
            UNREACHABLE();
            return;
    }
}

void ContextPrivateHint(Context *context, GLenum target, GLenum mode)
{
    switch (target)
    {
        case GL_GENERATE_MIPMAP_HINT:
            context->getMutablePrivateState()->setGenerateMipmapHint(mode);
            break;

        case GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES:
            context->getMutablePrivateState()->setFragmentShaderDerivativeHint(mode);
            break;

        case GL_PERSPECTIVE_CORRECTION_HINT:
        case GL_POINT_SMOOTH_HINT:
        case GL_LINE_SMOOTH_HINT:
        case GL_FOG_HINT:
            context->getMutableGLES1State()->setHint(target, mode);
            break;
        case GL_TEXTURE_FILTERING_HINT_CHROMIUM:
            context->getMutablePrivateState()->setTextureFilteringHint(mode);
            break;
        default:
            UNREACHABLE();
            return;
    }
}

GLboolean ContextPrivateIsEnabled(Context *context, GLenum cap)
{
    return context->getState().privateState().getEnableFeature(cap);
}

GLboolean ContextPrivateIsEnabledi(Context *context, GLenum target, GLuint index)
{
    return context->getState().privateState().getEnableFeatureIndexed(target, index);
}

void ContextPrivatePatchParameteri(Context *context, GLenum pname, GLint value)
{
    switch (pname)
    {
        case GL_PATCH_VERTICES:
            context->getMutablePrivateState()->setPatchVertices(value);
            break;
        default:
            break;
    }
}

void ContextPrivateAlphaFunc(Context *context, AlphaTestFunc func, GLfloat ref)
{
    context->getMutableGLES1State()->setAlphaTestParameters(func, ref);
}

void ContextPrivateAlphaFuncx(Context *context, AlphaTestFunc func, GLfixed ref)
{
    ContextPrivateAlphaFunc(context, func, ConvertFixedToFloat(ref));
}

void ContextPrivateClipPlanef(Context *context, GLenum p, const GLfloat *eqn)
{
    context->getMutableGLES1State()->setClipPlane(p - GL_CLIP_PLANE0, eqn);
}

void ContextPrivateClipPlanex(Context *context, GLenum plane, const GLfixed *equation)
{
    const GLfloat equationf[4] = {
        ConvertFixedToFloat(equation[0]),
        ConvertFixedToFloat(equation[1]),
        ConvertFixedToFloat(equation[2]),
        ConvertFixedToFloat(equation[3]),
    };

    ContextPrivateClipPlanef(context, plane, equationf);
}

void ContextPrivateColor4f(Context *context,
                           GLfloat red,
                           GLfloat green,
                           GLfloat blue,
                           GLfloat alpha)
{
    context->getMutableGLES1State()->setCurrentColor({red, green, blue, alpha});
}

void ContextPrivateColor4ub(Context *context,
                            GLubyte red,
                            GLubyte green,
                            GLubyte blue,
                            GLubyte alpha)
{
    ContextPrivateColor4f(context, normalizedToFloat<uint8_t>(red),
                          normalizedToFloat<uint8_t>(green), normalizedToFloat<uint8_t>(blue),
                          normalizedToFloat<uint8_t>(alpha));
}

void ContextPrivateColor4x(Context *context,
                           GLfixed red,
                           GLfixed green,
                           GLfixed blue,
                           GLfixed alpha)
{
    ContextPrivateColor4f(context, ConvertFixedToFloat(red), ConvertFixedToFloat(green),
                          ConvertFixedToFloat(blue), ConvertFixedToFloat(alpha));
}

void ContextPrivateFogf(Context *context, GLenum pname, GLfloat param)
{
    ContextPrivateFogfv(context, pname, &param);
}

void ContextPrivateFogfv(Context *context, GLenum pname, const GLfloat *params)
{
    SetFogParameters(context->getMutableGLES1State(), pname, params);
}

void ContextPrivateFogx(Context *context, GLenum pname, GLfixed param)
{
    if (GetFogParameterCount(pname) == 1)
    {
        GLfloat paramf = pname == GL_FOG_MODE ? ConvertToGLenum(param) : ConvertFixedToFloat(param);
        ContextPrivateFogfv(context, pname, &paramf);
    }
    else
    {
        UNREACHABLE();
    }
}

void ContextPrivateFogxv(Context *context, GLenum pname, const GLfixed *params)
{
    int paramCount = GetFogParameterCount(pname);

    if (paramCount > 0)
    {
        GLfloat paramsf[4];
        for (int i = 0; i < paramCount; i++)
        {
            paramsf[i] =
                pname == GL_FOG_MODE ? ConvertToGLenum(params[i]) : ConvertFixedToFloat(params[i]);
        }
        ContextPrivateFogfv(context, pname, paramsf);
    }
    else
    {
        UNREACHABLE();
    }
}

void ContextPrivateFrustumf(Context *context,
                            GLfloat l,
                            GLfloat r,
                            GLfloat b,
                            GLfloat t,
                            GLfloat n,
                            GLfloat f)
{
    context->getMutableGLES1State()->multMatrix(angle::Mat4::Frustum(l, r, b, t, n, f));
}

void ContextPrivateFrustumx(Context *context,
                            GLfixed l,
                            GLfixed r,
                            GLfixed b,
                            GLfixed t,
                            GLfixed n,
                            GLfixed f)
{
    ContextPrivateFrustumf(context, ConvertFixedToFloat(l), ConvertFixedToFloat(r),
                           ConvertFixedToFloat(b), ConvertFixedToFloat(t), ConvertFixedToFloat(n),
                           ConvertFixedToFloat(f));
}

void ContextPrivateGetClipPlanef(Context *context, GLenum plane, GLfloat *equation)
{
    context->getState().gles1().getClipPlane(plane - GL_CLIP_PLANE0, equation);
}

void ContextPrivateGetClipPlanex(Context *context, GLenum plane, GLfixed *equation)
{
    GLfloat equationf[4] = {};

    ContextPrivateGetClipPlanef(context, plane, equationf);

    for (int i = 0; i < 4; i++)
    {
        equation[i] = ConvertFloatToFixed(equationf[i]);
    }
}

void ContextPrivateGetLightfv(Context *context, GLenum light, LightParameter pname, GLfloat *params)
{
    GetLightParameters(context->getMutableGLES1State(), light, pname, params);
}

void ContextPrivateGetLightxv(Context *context, GLenum light, LightParameter pname, GLfixed *params)
{
    GLfloat paramsf[4];
    ContextPrivateGetLightfv(context, light, pname, paramsf);

    for (unsigned int i = 0; i < GetLightParameterCount(pname); i++)
    {
        params[i] = ConvertFloatToFixed(paramsf[i]);
    }
}

void ContextPrivateGetMaterialfv(Context *context,
                                 GLenum face,
                                 MaterialParameter pname,
                                 GLfloat *params)
{
    GetMaterialParameters(context->getMutableGLES1State(), face, pname, params);
}

void ContextPrivateGetMaterialxv(Context *context,
                                 GLenum face,
                                 MaterialParameter pname,
                                 GLfixed *params)
{
    GLfloat paramsf[4];
    ContextPrivateGetMaterialfv(context, face, pname, paramsf);

    for (unsigned int i = 0; i < GetMaterialParameterCount(pname); i++)
    {
        params[i] = ConvertFloatToFixed(paramsf[i]);
    }
}

void ContextPrivateGetTexEnvfv(Context *context,
                               TextureEnvTarget target,
                               TextureEnvParameter pname,
                               GLfloat *params)
{
    GetTextureEnv(context->getState().privateState().getActiveSampler(),
                  context->getMutableGLES1State(), target, pname, params);
}

void ContextPrivateGetTexEnviv(Context *context,
                               TextureEnvTarget target,
                               TextureEnvParameter pname,
                               GLint *params)
{
    GLfloat paramsf[4];
    ContextPrivateGetTexEnvfv(context, target, pname, paramsf);
    ConvertTextureEnvToInt(pname, paramsf, params);
}

void ContextPrivateGetTexEnvxv(Context *context,
                               TextureEnvTarget target,
                               TextureEnvParameter pname,
                               GLfixed *params)
{
    GLfloat paramsf[4];
    ContextPrivateGetTexEnvfv(context, target, pname, paramsf);
    ConvertTextureEnvToFixed(pname, paramsf, params);
}

void ContextPrivateLightModelf(Context *context, GLenum pname, GLfloat param)
{
    ContextPrivateLightModelfv(context, pname, &param);
}

void ContextPrivateLightModelfv(Context *context, GLenum pname, const GLfloat *params)
{
    SetLightModelParameters(context->getMutableGLES1State(), pname, params);
}

void ContextPrivateLightModelx(Context *context, GLenum pname, GLfixed param)
{
    ContextPrivateLightModelf(context, pname, ConvertFixedToFloat(param));
}

void ContextPrivateLightModelxv(Context *context, GLenum pname, const GLfixed *param)
{
    GLfloat paramsf[4];

    for (unsigned int i = 0; i < GetLightModelParameterCount(pname); i++)
    {
        paramsf[i] = ConvertFixedToFloat(param[i]);
    }

    ContextPrivateLightModelfv(context, pname, paramsf);
}

void ContextPrivateLightf(Context *context, GLenum light, LightParameter pname, GLfloat param)
{
    ContextPrivateLightfv(context, light, pname, &param);
}

void ContextPrivateLightfv(Context *context,
                           GLenum light,
                           LightParameter pname,
                           const GLfloat *params)
{
    SetLightParameters(context->getMutableGLES1State(), light, pname, params);
}

void ContextPrivateLightx(Context *context, GLenum light, LightParameter pname, GLfixed param)
{
    ContextPrivateLightf(context, light, pname, ConvertFixedToFloat(param));
}

void ContextPrivateLightxv(Context *context,
                           GLenum light,
                           LightParameter pname,
                           const GLfixed *params)
{
    GLfloat paramsf[4];

    for (unsigned int i = 0; i < GetLightParameterCount(pname); i++)
    {
        paramsf[i] = ConvertFixedToFloat(params[i]);
    }

    ContextPrivateLightfv(context, light, pname, paramsf);
}

void ContextPrivateLoadIdentity(Context *context)
{
    context->getMutableGLES1State()->loadMatrix(angle::Mat4());
}

void ContextPrivateLoadMatrixf(Context *context, const GLfloat *m)
{
    context->getMutableGLES1State()->loadMatrix(angle::Mat4(m));
}

void ContextPrivateLoadMatrixx(Context *context, const GLfixed *m)
{
    context->getMutableGLES1State()->loadMatrix(FixedMatrixToMat4(m));
}

void ContextPrivateMaterialf(Context *context, GLenum face, MaterialParameter pname, GLfloat param)
{
    ContextPrivateMaterialfv(context, face, pname, &param);
}

void ContextPrivateMaterialfv(Context *context,
                              GLenum face,
                              MaterialParameter pname,
                              const GLfloat *params)
{
    SetMaterialParameters(context->getMutableGLES1State(), face, pname, params);
}

void ContextPrivateMaterialx(Context *context, GLenum face, MaterialParameter pname, GLfixed param)
{
    ContextPrivateMaterialf(context, face, pname, ConvertFixedToFloat(param));
}

void ContextPrivateMaterialxv(Context *context,
                              GLenum face,
                              MaterialParameter pname,
                              const GLfixed *param)
{
    GLfloat paramsf[4];

    for (unsigned int i = 0; i < GetMaterialParameterCount(pname); i++)
    {
        paramsf[i] = ConvertFixedToFloat(param[i]);
    }

    ContextPrivateMaterialfv(context, face, pname, paramsf);
}

void ContextPrivateMatrixMode(Context *context, MatrixType mode)
{
    context->getMutableGLES1State()->setMatrixMode(mode);
}

void ContextPrivateMultMatrixf(Context *context, const GLfloat *m)
{
    context->getMutableGLES1State()->multMatrix(angle::Mat4(m));
}

void ContextPrivateMultMatrixx(Context *context, const GLfixed *m)
{
    context->getMutableGLES1State()->multMatrix(FixedMatrixToMat4(m));
}

void ContextPrivateMultiTexCoord4f(Context *context,
                                   GLenum target,
                                   GLfloat s,
                                   GLfloat t,
                                   GLfloat r,
                                   GLfloat q)
{
    unsigned int unit = target - GL_TEXTURE0;
    ASSERT(target >= GL_TEXTURE0 &&
           unit < context->getState().privateState().getCaps().maxMultitextureUnits);
    context->getMutableGLES1State()->setCurrentTextureCoords(unit, {s, t, r, q});
}

void ContextPrivateMultiTexCoord4x(Context *context,
                                   GLenum texture,
                                   GLfixed s,
                                   GLfixed t,
                                   GLfixed r,
                                   GLfixed q)
{
    ContextPrivateMultiTexCoord4f(context, texture, ConvertFixedToFloat(s), ConvertFixedToFloat(t),
                                  ConvertFixedToFloat(r), ConvertFixedToFloat(q));
}

void ContextPrivateNormal3f(Context *context, GLfloat nx, GLfloat ny, GLfloat nz)
{
    context->getMutableGLES1State()->setCurrentNormal({nx, ny, nz});
}

void ContextPrivateNormal3x(Context *context, GLfixed nx, GLfixed ny, GLfixed nz)
{
    ContextPrivateNormal3f(context, ConvertFixedToFloat(nx), ConvertFixedToFloat(ny),
                           ConvertFixedToFloat(nz));
}

void ContextPrivateOrthof(Context *context,
                          GLfloat left,
                          GLfloat right,
                          GLfloat bottom,
                          GLfloat top,
                          GLfloat zNear,
                          GLfloat zFar)
{
    context->getMutableGLES1State()->multMatrix(
        angle::Mat4::Ortho(left, right, bottom, top, zNear, zFar));
}

void ContextPrivateOrthox(Context *context,
                          GLfixed left,
                          GLfixed right,
                          GLfixed bottom,
                          GLfixed top,
                          GLfixed zNear,
                          GLfixed zFar)
{
    ContextPrivateOrthof(context, ConvertFixedToFloat(left), ConvertFixedToFloat(right),
                         ConvertFixedToFloat(bottom), ConvertFixedToFloat(top),
                         ConvertFixedToFloat(zNear), ConvertFixedToFloat(zFar));
}

void ContextPrivatePointParameterf(Context *context, PointParameter pname, GLfloat param)
{
    ContextPrivatePointParameterfv(context, pname, &param);
}

void ContextPrivatePointParameterfv(Context *context, PointParameter pname, const GLfloat *params)
{
    SetPointParameter(context->getMutableGLES1State(), pname, params);
}

void ContextPrivatePointParameterx(Context *context, PointParameter pname, GLfixed param)
{
    ContextPrivatePointParameterf(context, pname, ConvertFixedToFloat(param));
}

void ContextPrivatePointParameterxv(Context *context, PointParameter pname, const GLfixed *params)
{
    GLfloat paramsf[4] = {};
    for (unsigned int i = 0; i < GetPointParameterCount(pname); i++)
    {
        paramsf[i] = ConvertFixedToFloat(params[i]);
    }
    ContextPrivatePointParameterfv(context, pname, paramsf);
}

void ContextPrivatePointSize(Context *context, GLfloat size)
{
    SetPointSize(context->getMutableGLES1State(), size);
}

void ContextPrivatePointSizex(Context *context, GLfixed size)
{
    ContextPrivatePointSize(context, ConvertFixedToFloat(size));
}

void ContextPrivatePopMatrix(Context *context)
{
    context->getMutableGLES1State()->popMatrix();
}

void ContextPrivatePushMatrix(Context *context)
{
    context->getMutableGLES1State()->pushMatrix();
}

void ContextPrivateRotatef(Context *context, GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    context->getMutableGLES1State()->multMatrix(
        angle::Mat4::Rotate(angle, angle::Vector3(x, y, z)));
}

void ContextPrivateRotatex(Context *context, GLfixed angle, GLfixed x, GLfixed y, GLfixed z)
{
    ContextPrivateRotatef(context, ConvertFixedToFloat(angle), ConvertFixedToFloat(x),
                          ConvertFixedToFloat(y), ConvertFixedToFloat(z));
}

void ContextPrivateScalef(Context *context, GLfloat x, GLfloat y, GLfloat z)
{
    context->getMutableGLES1State()->multMatrix(angle::Mat4::Scale(angle::Vector3(x, y, z)));
}

void ContextPrivateScalex(Context *context, GLfixed x, GLfixed y, GLfixed z)
{
    ContextPrivateScalef(context, ConvertFixedToFloat(x), ConvertFixedToFloat(y),
                         ConvertFixedToFloat(z));
}

void ContextPrivateShadeModel(Context *context, ShadingModel model)
{
    context->getMutableGLES1State()->setShadeModel(model);
}

void ContextPrivateTexEnvf(Context *context,
                           TextureEnvTarget target,
                           TextureEnvParameter pname,
                           GLfloat param)
{
    ContextPrivateTexEnvfv(context, target, pname, &param);
}

void ContextPrivateTexEnvfv(Context *context,
                            TextureEnvTarget target,
                            TextureEnvParameter pname,
                            const GLfloat *params)
{
    SetTextureEnv(context->getState().privateState().getActiveSampler(),
                  context->getMutableGLES1State(), target, pname, params);
}

void ContextPrivateTexEnvi(Context *context,
                           TextureEnvTarget target,
                           TextureEnvParameter pname,
                           GLint param)
{
    ContextPrivateTexEnviv(context, target, pname, &param);
}

void ContextPrivateTexEnviv(Context *context,
                            TextureEnvTarget target,
                            TextureEnvParameter pname,
                            const GLint *params)
{
    GLfloat paramsf[4] = {};
    ConvertTextureEnvFromInt(pname, params, paramsf);
    ContextPrivateTexEnvfv(context, target, pname, paramsf);
}

void ContextPrivateTexEnvx(Context *context,
                           TextureEnvTarget target,
                           TextureEnvParameter pname,
                           GLfixed param)
{
    ContextPrivateTexEnvxv(context, target, pname, &param);
}

void ContextPrivateTexEnvxv(Context *context,
                            TextureEnvTarget target,
                            TextureEnvParameter pname,
                            const GLfixed *params)
{
    GLfloat paramsf[4] = {};
    ConvertTextureEnvFromFixed(pname, params, paramsf);
    ContextPrivateTexEnvfv(context, target, pname, paramsf);
}

void ContextPrivateTranslatef(Context *context, GLfloat x, GLfloat y, GLfloat z)
{
    context->getMutableGLES1State()->multMatrix(angle::Mat4::Translate(angle::Vector3(x, y, z)));
}

void ContextPrivateTranslatex(Context *context, GLfixed x, GLfixed y, GLfixed z)
{
    ContextPrivateTranslatef(context, ConvertFixedToFloat(x), ConvertFixedToFloat(y),
                             ConvertFixedToFloat(z));
}
}  // namespace gl
