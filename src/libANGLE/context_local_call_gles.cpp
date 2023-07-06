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
    ContextLocalClearColor(context, ConvertFixedToFloat(red), ConvertFixedToFloat(green),
                           ConvertFixedToFloat(blue), ConvertFixedToFloat(alpha));
}

void ContextLocalClearDepthx(Context *context, GLfixed depth)
{
    ContextLocalClearDepthf(context, ConvertFixedToFloat(depth));
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
    ContextLocalDepthRangef(context, ConvertFixedToFloat(zNear), ConvertFixedToFloat(zFar));
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
    ContextLocalLineWidth(context, ConvertFixedToFloat(width));
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
    ContextLocalPolygonOffsetClamp(context, ConvertFixedToFloat(factor), ConvertFixedToFloat(units),
                                   0.0f);
}

void ContextLocalSampleCoverage(Context *context, GLfloat value, GLboolean invert)
{
    context->getMutableLocalState()->setSampleCoverageParams(clamp01(value), ConvertToBool(invert));
}

void ContextLocalSampleCoveragex(Context *context, GLclampx value, GLboolean invert)
{
    ContextLocalSampleCoverage(context, ConvertFixedToFloat(value), invert);
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

void ContextLocalAlphaFunc(Context *context, AlphaTestFunc func, GLfloat ref)
{
    context->getMutableGLES1State()->setAlphaTestParameters(func, ref);
}

void ContextLocalAlphaFuncx(Context *context, AlphaTestFunc func, GLfixed ref)
{
    ContextLocalAlphaFunc(context, func, ConvertFixedToFloat(ref));
}

void ContextLocalClipPlanef(Context *context, GLenum p, const GLfloat *eqn)
{
    context->getMutableGLES1State()->setClipPlane(p - GL_CLIP_PLANE0, eqn);
}

void ContextLocalClipPlanex(Context *context, GLenum plane, const GLfixed *equation)
{
    const GLfloat equationf[4] = {
        ConvertFixedToFloat(equation[0]),
        ConvertFixedToFloat(equation[1]),
        ConvertFixedToFloat(equation[2]),
        ConvertFixedToFloat(equation[3]),
    };

    ContextLocalClipPlanef(context, plane, equationf);
}

void ContextLocalColor4f(Context *context, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    context->getMutableGLES1State()->setCurrentColor({red, green, blue, alpha});
}

void ContextLocalColor4ub(Context *context, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    ContextLocalColor4f(context, normalizedToFloat<uint8_t>(red), normalizedToFloat<uint8_t>(green),
                        normalizedToFloat<uint8_t>(blue), normalizedToFloat<uint8_t>(alpha));
}

void ContextLocalColor4x(Context *context, GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha)
{
    ContextLocalColor4f(context, ConvertFixedToFloat(red), ConvertFixedToFloat(green),
                        ConvertFixedToFloat(blue), ConvertFixedToFloat(alpha));
}

void ContextLocalFogf(Context *context, GLenum pname, GLfloat param)
{
    ContextLocalFogfv(context, pname, &param);
}

void ContextLocalFogfv(Context *context, GLenum pname, const GLfloat *params)
{
    SetFogParameters(context->getMutableGLES1State(), pname, params);
}

void ContextLocalFogx(Context *context, GLenum pname, GLfixed param)
{
    if (GetFogParameterCount(pname) == 1)
    {
        GLfloat paramf = pname == GL_FOG_MODE ? ConvertToGLenum(param) : ConvertFixedToFloat(param);
        ContextLocalFogfv(context, pname, &paramf);
    }
    else
    {
        UNREACHABLE();
    }
}

void ContextLocalFogxv(Context *context, GLenum pname, const GLfixed *params)
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
        ContextLocalFogfv(context, pname, paramsf);
    }
    else
    {
        UNREACHABLE();
    }
}

void ContextLocalFrustumf(Context *context,
                          GLfloat l,
                          GLfloat r,
                          GLfloat b,
                          GLfloat t,
                          GLfloat n,
                          GLfloat f)
{
    context->getMutableGLES1State()->multMatrix(angle::Mat4::Frustum(l, r, b, t, n, f));
}

void ContextLocalFrustumx(Context *context,
                          GLfixed l,
                          GLfixed r,
                          GLfixed b,
                          GLfixed t,
                          GLfixed n,
                          GLfixed f)
{
    ContextLocalFrustumf(context, ConvertFixedToFloat(l), ConvertFixedToFloat(r),
                         ConvertFixedToFloat(b), ConvertFixedToFloat(t), ConvertFixedToFloat(n),
                         ConvertFixedToFloat(f));
}

void ContextLocalGetClipPlanef(Context *context, GLenum plane, GLfloat *equation)
{
    context->getState().gles1().getClipPlane(plane - GL_CLIP_PLANE0, equation);
}

void ContextLocalGetClipPlanex(Context *context, GLenum plane, GLfixed *equation)
{
    GLfloat equationf[4] = {};

    ContextLocalGetClipPlanef(context, plane, equationf);

    for (int i = 0; i < 4; i++)
    {
        equation[i] = ConvertFloatToFixed(equationf[i]);
    }
}

void ContextLocalGetLightfv(Context *context, GLenum light, LightParameter pname, GLfloat *params)
{
    GetLightParameters(context->getMutableGLES1State(), light, pname, params);
}

void ContextLocalGetLightxv(Context *context, GLenum light, LightParameter pname, GLfixed *params)
{
    GLfloat paramsf[4];
    ContextLocalGetLightfv(context, light, pname, paramsf);

    for (unsigned int i = 0; i < GetLightParameterCount(pname); i++)
    {
        params[i] = ConvertFloatToFixed(paramsf[i]);
    }
}

void ContextLocalGetMaterialfv(Context *context,
                               GLenum face,
                               MaterialParameter pname,
                               GLfloat *params)
{
    GetMaterialParameters(context->getMutableGLES1State(), face, pname, params);
}

void ContextLocalGetMaterialxv(Context *context,
                               GLenum face,
                               MaterialParameter pname,
                               GLfixed *params)
{
    GLfloat paramsf[4];
    ContextLocalGetMaterialfv(context, face, pname, paramsf);

    for (unsigned int i = 0; i < GetMaterialParameterCount(pname); i++)
    {
        params[i] = ConvertFloatToFixed(paramsf[i]);
    }
}

void ContextLocalGetTexEnvfv(Context *context,
                             TextureEnvTarget target,
                             TextureEnvParameter pname,
                             GLfloat *params)
{
    GetTextureEnv(context->getState().localState().getActiveSampler(),
                  context->getMutableGLES1State(), target, pname, params);
}

void ContextLocalGetTexEnviv(Context *context,
                             TextureEnvTarget target,
                             TextureEnvParameter pname,
                             GLint *params)
{
    GLfloat paramsf[4];
    ContextLocalGetTexEnvfv(context, target, pname, paramsf);
    ConvertTextureEnvToInt(pname, paramsf, params);
}

void ContextLocalGetTexEnvxv(Context *context,
                             TextureEnvTarget target,
                             TextureEnvParameter pname,
                             GLfixed *params)
{
    GLfloat paramsf[4];
    ContextLocalGetTexEnvfv(context, target, pname, paramsf);
    ConvertTextureEnvToFixed(pname, paramsf, params);
}

void ContextLocalLightModelf(Context *context, GLenum pname, GLfloat param)
{
    ContextLocalLightModelfv(context, pname, &param);
}

void ContextLocalLightModelfv(Context *context, GLenum pname, const GLfloat *params)
{
    SetLightModelParameters(context->getMutableGLES1State(), pname, params);
}

void ContextLocalLightModelx(Context *context, GLenum pname, GLfixed param)
{
    ContextLocalLightModelf(context, pname, ConvertFixedToFloat(param));
}

void ContextLocalLightModelxv(Context *context, GLenum pname, const GLfixed *param)
{
    GLfloat paramsf[4];

    for (unsigned int i = 0; i < GetLightModelParameterCount(pname); i++)
    {
        paramsf[i] = ConvertFixedToFloat(param[i]);
    }

    ContextLocalLightModelfv(context, pname, paramsf);
}

void ContextLocalLightf(Context *context, GLenum light, LightParameter pname, GLfloat param)
{
    ContextLocalLightfv(context, light, pname, &param);
}

void ContextLocalLightfv(Context *context,
                         GLenum light,
                         LightParameter pname,
                         const GLfloat *params)
{
    SetLightParameters(context->getMutableGLES1State(), light, pname, params);
}

void ContextLocalLightx(Context *context, GLenum light, LightParameter pname, GLfixed param)
{
    ContextLocalLightf(context, light, pname, ConvertFixedToFloat(param));
}

void ContextLocalLightxv(Context *context,
                         GLenum light,
                         LightParameter pname,
                         const GLfixed *params)
{
    GLfloat paramsf[4];

    for (unsigned int i = 0; i < GetLightParameterCount(pname); i++)
    {
        paramsf[i] = ConvertFixedToFloat(params[i]);
    }

    ContextLocalLightfv(context, light, pname, paramsf);
}

void ContextLocalLoadIdentity(Context *context)
{
    context->getMutableGLES1State()->loadMatrix(angle::Mat4());
}

void ContextLocalLoadMatrixf(Context *context, const GLfloat *m)
{
    context->getMutableGLES1State()->loadMatrix(angle::Mat4(m));
}

void ContextLocalLoadMatrixx(Context *context, const GLfixed *m)
{
    context->getMutableGLES1State()->loadMatrix(FixedMatrixToMat4(m));
}

void ContextLocalMaterialf(Context *context, GLenum face, MaterialParameter pname, GLfloat param)
{
    ContextLocalMaterialfv(context, face, pname, &param);
}

void ContextLocalMaterialfv(Context *context,
                            GLenum face,
                            MaterialParameter pname,
                            const GLfloat *params)
{
    SetMaterialParameters(context->getMutableGLES1State(), face, pname, params);
}

void ContextLocalMaterialx(Context *context, GLenum face, MaterialParameter pname, GLfixed param)
{
    ContextLocalMaterialf(context, face, pname, ConvertFixedToFloat(param));
}

void ContextLocalMaterialxv(Context *context,
                            GLenum face,
                            MaterialParameter pname,
                            const GLfixed *param)
{
    GLfloat paramsf[4];

    for (unsigned int i = 0; i < GetMaterialParameterCount(pname); i++)
    {
        paramsf[i] = ConvertFixedToFloat(param[i]);
    }

    ContextLocalMaterialfv(context, face, pname, paramsf);
}

void ContextLocalMatrixMode(Context *context, MatrixType mode)
{
    context->getMutableGLES1State()->setMatrixMode(mode);
}

void ContextLocalMultMatrixf(Context *context, const GLfloat *m)
{
    context->getMutableGLES1State()->multMatrix(angle::Mat4(m));
}

void ContextLocalMultMatrixx(Context *context, const GLfixed *m)
{
    context->getMutableGLES1State()->multMatrix(FixedMatrixToMat4(m));
}

void ContextLocalMultiTexCoord4f(Context *context,
                                 GLenum target,
                                 GLfloat s,
                                 GLfloat t,
                                 GLfloat r,
                                 GLfloat q)
{
    unsigned int unit = target - GL_TEXTURE0;
    ASSERT(target >= GL_TEXTURE0 &&
           unit < context->getState().localState().getCaps().maxMultitextureUnits);
    context->getMutableGLES1State()->setCurrentTextureCoords(unit, {s, t, r, q});
}

void ContextLocalMultiTexCoord4x(Context *context,
                                 GLenum texture,
                                 GLfixed s,
                                 GLfixed t,
                                 GLfixed r,
                                 GLfixed q)
{
    ContextLocalMultiTexCoord4f(context, texture, ConvertFixedToFloat(s), ConvertFixedToFloat(t),
                                ConvertFixedToFloat(r), ConvertFixedToFloat(q));
}

void ContextLocalNormal3f(Context *context, GLfloat nx, GLfloat ny, GLfloat nz)
{
    context->getMutableGLES1State()->setCurrentNormal({nx, ny, nz});
}

void ContextLocalNormal3x(Context *context, GLfixed nx, GLfixed ny, GLfixed nz)
{
    ContextLocalNormal3f(context, ConvertFixedToFloat(nx), ConvertFixedToFloat(ny),
                         ConvertFixedToFloat(nz));
}

void ContextLocalOrthof(Context *context,
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

void ContextLocalOrthox(Context *context,
                        GLfixed left,
                        GLfixed right,
                        GLfixed bottom,
                        GLfixed top,
                        GLfixed zNear,
                        GLfixed zFar)
{
    ContextLocalOrthof(context, ConvertFixedToFloat(left), ConvertFixedToFloat(right),
                       ConvertFixedToFloat(bottom), ConvertFixedToFloat(top),
                       ConvertFixedToFloat(zNear), ConvertFixedToFloat(zFar));
}

void ContextLocalPointParameterf(Context *context, PointParameter pname, GLfloat param)
{
    ContextLocalPointParameterfv(context, pname, &param);
}

void ContextLocalPointParameterfv(Context *context, PointParameter pname, const GLfloat *params)
{
    SetPointParameter(context->getMutableGLES1State(), pname, params);
}

void ContextLocalPointParameterx(Context *context, PointParameter pname, GLfixed param)
{
    ContextLocalPointParameterf(context, pname, ConvertFixedToFloat(param));
}

void ContextLocalPointParameterxv(Context *context, PointParameter pname, const GLfixed *params)
{
    GLfloat paramsf[4] = {};
    for (unsigned int i = 0; i < GetPointParameterCount(pname); i++)
    {
        paramsf[i] = ConvertFixedToFloat(params[i]);
    }
    ContextLocalPointParameterfv(context, pname, paramsf);
}

void ContextLocalPointSize(Context *context, GLfloat size)
{
    SetPointSize(context->getMutableGLES1State(), size);
}

void ContextLocalPointSizex(Context *context, GLfixed size)
{
    ContextLocalPointSize(context, ConvertFixedToFloat(size));
}

void ContextLocalPopMatrix(Context *context)
{
    context->getMutableGLES1State()->popMatrix();
}

void ContextLocalPushMatrix(Context *context)
{
    context->getMutableGLES1State()->pushMatrix();
}

void ContextLocalRotatef(Context *context, GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    context->getMutableGLES1State()->multMatrix(
        angle::Mat4::Rotate(angle, angle::Vector3(x, y, z)));
}

void ContextLocalRotatex(Context *context, GLfixed angle, GLfixed x, GLfixed y, GLfixed z)
{
    ContextLocalRotatef(context, ConvertFixedToFloat(angle), ConvertFixedToFloat(x),
                        ConvertFixedToFloat(y), ConvertFixedToFloat(z));
}

void ContextLocalScalef(Context *context, GLfloat x, GLfloat y, GLfloat z)
{
    context->getMutableGLES1State()->multMatrix(angle::Mat4::Scale(angle::Vector3(x, y, z)));
}

void ContextLocalScalex(Context *context, GLfixed x, GLfixed y, GLfixed z)
{
    ContextLocalScalef(context, ConvertFixedToFloat(x), ConvertFixedToFloat(y),
                       ConvertFixedToFloat(z));
}

void ContextLocalShadeModel(Context *context, ShadingModel model)
{
    context->getMutableGLES1State()->setShadeModel(model);
}

void ContextLocalTexEnvf(Context *context,
                         TextureEnvTarget target,
                         TextureEnvParameter pname,
                         GLfloat param)
{
    ContextLocalTexEnvfv(context, target, pname, &param);
}

void ContextLocalTexEnvfv(Context *context,
                          TextureEnvTarget target,
                          TextureEnvParameter pname,
                          const GLfloat *params)
{
    SetTextureEnv(context->getState().localState().getActiveSampler(),
                  context->getMutableGLES1State(), target, pname, params);
}

void ContextLocalTexEnvi(Context *context,
                         TextureEnvTarget target,
                         TextureEnvParameter pname,
                         GLint param)
{
    ContextLocalTexEnviv(context, target, pname, &param);
}

void ContextLocalTexEnviv(Context *context,
                          TextureEnvTarget target,
                          TextureEnvParameter pname,
                          const GLint *params)
{
    GLfloat paramsf[4] = {};
    ConvertTextureEnvFromInt(pname, params, paramsf);
    ContextLocalTexEnvfv(context, target, pname, paramsf);
}

void ContextLocalTexEnvx(Context *context,
                         TextureEnvTarget target,
                         TextureEnvParameter pname,
                         GLfixed param)
{
    ContextLocalTexEnvxv(context, target, pname, &param);
}

void ContextLocalTexEnvxv(Context *context,
                          TextureEnvTarget target,
                          TextureEnvParameter pname,
                          const GLfixed *params)
{
    GLfloat paramsf[4] = {};
    ConvertTextureEnvFromFixed(pname, params, paramsf);
    ContextLocalTexEnvfv(context, target, pname, paramsf);
}

void ContextLocalTranslatef(Context *context, GLfloat x, GLfloat y, GLfloat z)
{
    context->getMutableGLES1State()->multMatrix(angle::Mat4::Translate(angle::Vector3(x, y, z)));
}

void ContextLocalTranslatex(Context *context, GLfixed x, GLfixed y, GLfixed z)
{
    ContextLocalTranslatef(context, ConvertFixedToFloat(x), ConvertFixedToFloat(y),
                           ConvertFixedToFloat(z));
}
}  // namespace gl
