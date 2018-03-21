//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Context_gles_1_0.cpp: Implements the GLES1-specific parts of Context.

#include "libANGLE/Context.h"

#include "common/mathutil.h"
#include "common/utilities.h"

namespace gl
{

void Context::alphaFunc(AlphaTestFunc func, GLfloat ref)
{
    mGLState.gles1().setAlphaFunc(func, ref);
}

void Context::alphaFuncx(AlphaTestFunc func, GLfixed ref)
{
    mGLState.gles1().setAlphaFunc(func, FixedToFloat(ref));
}

void Context::clearColorx(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha)
{
    UNIMPLEMENTED();
}

void Context::clearDepthx(GLfixed depth)
{
    UNIMPLEMENTED();
}

void Context::clientActiveTexture(GLenum texture)
{
    UNIMPLEMENTED();
}

void Context::clipPlanef(GLenum p, const GLfloat *eqn)
{
    UNIMPLEMENTED();
}

void Context::clipPlanex(GLenum plane, const GLfixed *equation)
{
    UNIMPLEMENTED();
}

void Context::color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    UNIMPLEMENTED();
}

void Context::color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    UNIMPLEMENTED();
}

void Context::color4x(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha)
{
    UNIMPLEMENTED();
}

void Context::colorPointer(GLint size, GLenum type, GLsizei stride, const void *ptr)
{
    UNIMPLEMENTED();
}

void Context::depthRangex(GLfixed n, GLfixed f)
{
    UNIMPLEMENTED();
}

void Context::disableClientState(GLenum clientState)
{
    UNIMPLEMENTED();
}

void Context::enableClientState(GLenum clientState)
{
    UNIMPLEMENTED();
}

void Context::fogf(GLenum pname, GLfloat param)
{
    UNIMPLEMENTED();
}

void Context::fogfv(GLenum pname, const GLfloat *params)
{
    UNIMPLEMENTED();
}

void Context::fogx(GLenum pname, GLfixed param)
{
    UNIMPLEMENTED();
}

void Context::fogxv(GLenum pname, const GLfixed *param)
{
    UNIMPLEMENTED();
}

void Context::frustumf(GLfloat left,
                       GLfloat right,
                       GLfloat bottom,
                       GLfloat top,
                       GLfloat zNear,
                       GLfloat zFar)
{
    UNIMPLEMENTED();
}

void Context::frustumx(GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f)
{
    UNIMPLEMENTED();
}

void Context::getClipPlanef(GLenum plane, GLfloat *equation)
{
    UNIMPLEMENTED();
}

void Context::getClipPlanex(GLenum plane, GLfixed *equation)
{
    UNIMPLEMENTED();
}

void Context::getFixedv(GLenum pname, GLfixed *params)
{
    UNIMPLEMENTED();
}

void Context::getLightfv(GLenum light, GLenum pname, GLfloat *params)
{
    UNIMPLEMENTED();
}

void Context::getLightxv(GLenum light, GLenum pname, GLfixed *params)
{
    UNIMPLEMENTED();
}

void Context::getMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
    UNIMPLEMENTED();
}

void Context::getMaterialxv(GLenum face, GLenum pname, GLfixed *params)
{
    UNIMPLEMENTED();
}

void Context::getTexEnvfv(GLenum env, GLenum pname, GLfloat *params)
{
    UNIMPLEMENTED();
}

void Context::getTexEnviv(GLenum env, GLenum pname, GLint *params)
{
    UNIMPLEMENTED();
}

void Context::getTexEnvxv(GLenum target, GLenum pname, GLfixed *params)
{
    UNIMPLEMENTED();
}

void Context::getTexParameterxv(TextureType target, GLenum pname, GLfixed *params)
{
    UNIMPLEMENTED();
}

void Context::lightModelf(GLenum pname, GLfloat param)
{
    UNIMPLEMENTED();
}

void Context::lightModelfv(GLenum pname, const GLfloat *params)
{
    UNIMPLEMENTED();
}

void Context::lightModelx(GLenum pname, GLfixed param)
{
    UNIMPLEMENTED();
}

void Context::lightModelxv(GLenum pname, const GLfixed *param)
{
    UNIMPLEMENTED();
}

void Context::lightf(GLenum light, GLenum pname, GLfloat param)
{
    UNIMPLEMENTED();
}

void Context::lightfv(GLenum light, GLenum pname, const GLfloat *params)
{
    UNIMPLEMENTED();
}

void Context::lightx(GLenum light, GLenum pname, GLfixed param)
{
    UNIMPLEMENTED();
}

void Context::lightxv(GLenum light, GLenum pname, const GLfixed *params)
{
    UNIMPLEMENTED();
}

void Context::lineWidthx(GLfixed width)
{
    UNIMPLEMENTED();
}

void Context::loadIdentity()
{
    UNIMPLEMENTED();
}

void Context::loadMatrixf(const GLfloat *m)
{
    UNIMPLEMENTED();
}

void Context::loadMatrixx(const GLfixed *m)
{
    UNIMPLEMENTED();
}

void Context::logicOp(GLenum opcode)
{
    UNIMPLEMENTED();
}

void Context::materialf(GLenum face, GLenum pname, GLfloat param)
{
    UNIMPLEMENTED();
}

void Context::materialfv(GLenum face, GLenum pname, const GLfloat *params)
{
    UNIMPLEMENTED();
}

void Context::materialx(GLenum face, GLenum pname, GLfixed param)
{
    UNIMPLEMENTED();
}

void Context::materialxv(GLenum face, GLenum pname, const GLfixed *param)
{
    UNIMPLEMENTED();
}

void Context::matrixMode(GLenum mode)
{
    UNIMPLEMENTED();
}

void Context::multMatrixf(const GLfloat *m)
{
    UNIMPLEMENTED();
}

void Context::multMatrixx(const GLfixed *m)
{
    UNIMPLEMENTED();
}

void Context::multiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    UNIMPLEMENTED();
}

void Context::multiTexCoord4x(GLenum texture, GLfixed s, GLfixed t, GLfixed r, GLfixed q)
{
    UNIMPLEMENTED();
}

void Context::normal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
    UNIMPLEMENTED();
}

void Context::normal3x(GLfixed nx, GLfixed ny, GLfixed nz)
{
    UNIMPLEMENTED();
}

void Context::normalPointer(GLenum type, GLsizei stride, const void *ptr)
{
    UNIMPLEMENTED();
}

void Context::orthof(GLfloat left,
                     GLfloat right,
                     GLfloat bottom,
                     GLfloat top,
                     GLfloat zNear,
                     GLfloat zFar)
{
    UNIMPLEMENTED();
}

void Context::orthox(GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f)
{
    UNIMPLEMENTED();
}

void Context::pointParameterf(GLenum pname, GLfloat param)
{
    UNIMPLEMENTED();
}

void Context::pointParameterfv(GLenum pname, const GLfloat *params)
{
    UNIMPLEMENTED();
}

void Context::pointParameterx(GLenum pname, GLfixed param)
{
    UNIMPLEMENTED();
}

void Context::pointParameterxv(GLenum pname, const GLfixed *params)
{
    UNIMPLEMENTED();
}

void Context::pointSize(GLfloat size)
{
    UNIMPLEMENTED();
}

void Context::pointSizex(GLfixed size)
{
    UNIMPLEMENTED();
}

void Context::polygonOffsetx(GLfixed factor, GLfixed units)
{
    UNIMPLEMENTED();
}

void Context::popMatrix()
{
    UNIMPLEMENTED();
}

void Context::pushMatrix()
{
    UNIMPLEMENTED();
}

void Context::rotatef(float angle, float x, float y, float z)
{
    UNIMPLEMENTED();
}

void Context::rotatex(GLfixed angle, GLfixed x, GLfixed y, GLfixed z)
{
    UNIMPLEMENTED();
}

void Context::sampleCoveragex(GLclampx value, GLboolean invert)
{
    UNIMPLEMENTED();
}

void Context::scalef(float x, float y, float z)
{
    UNIMPLEMENTED();
}

void Context::scalex(GLfixed x, GLfixed y, GLfixed z)
{
    UNIMPLEMENTED();
}

void Context::shadeModel(GLenum mode)
{
    UNIMPLEMENTED();
}

void Context::texCoordPointer(GLint size, GLenum type, GLsizei stride, const void *ptr)
{
    UNIMPLEMENTED();
}

void Context::texEnvf(GLenum target, GLenum pname, GLfloat param)
{
    UNIMPLEMENTED();
}

void Context::texEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
    UNIMPLEMENTED();
}

void Context::texEnvi(GLenum target, GLenum pname, GLint param)
{
    UNIMPLEMENTED();
}

void Context::texEnviv(GLenum target, GLenum pname, const GLint *params)
{
    UNIMPLEMENTED();
}

void Context::texEnvx(GLenum target, GLenum pname, GLfixed param)
{
    UNIMPLEMENTED();
}

void Context::texEnvxv(GLenum target, GLenum pname, const GLfixed *params)
{
    UNIMPLEMENTED();
}

void Context::texParameterx(TextureType target, GLenum pname, GLfixed param)
{
    UNIMPLEMENTED();
}

void Context::texParameterxv(TextureType target, GLenum pname, const GLfixed *params)
{
    UNIMPLEMENTED();
}

void Context::translatef(float x, float y, float z)
{
    UNIMPLEMENTED();
}

void Context::translatex(GLfixed x, GLfixed y, GLfixed z)
{
    UNIMPLEMENTED();
}

void Context::vertexPointer(GLint size, GLenum type, GLsizei stride, const void *ptr)
{
    UNIMPLEMENTED();
}

// GL_OES_draw_texture
void Context::drawTexf(float x, float y, float z, float width, float height)
{
    UNIMPLEMENTED();
}

void Context::drawTexfv(const GLfloat *coords)
{
    UNIMPLEMENTED();
}

void Context::drawTexi(GLint x, GLint y, GLint z, GLint width, GLint height)
{
    UNIMPLEMENTED();
}

void Context::drawTexiv(const GLint *coords)
{
    UNIMPLEMENTED();
}

void Context::drawTexs(GLshort x, GLshort y, GLshort z, GLshort width, GLshort height)
{
    UNIMPLEMENTED();
}

void Context::drawTexsv(const GLshort *coords)
{
    UNIMPLEMENTED();
}

void Context::drawTexx(GLfixed x, GLfixed y, GLfixed z, GLfixed width, GLfixed height)
{
    UNIMPLEMENTED();
}

void Context::drawTexxv(const GLfixed *coords)
{
    UNIMPLEMENTED();
}

// GL_OES_matrix_palette
void Context::currentPaletteMatrix(GLuint matrixpaletteindex)
{
    UNIMPLEMENTED();
}

void Context::loadPaletteFromModelViewMatrix()
{
    UNIMPLEMENTED();
}

void Context::matrixIndexPointer(GLint size, GLenum type, GLsizei stride, const void *pointer)
{
    UNIMPLEMENTED();
}

void Context::weightPointer(GLint size, GLenum type, GLsizei stride, const void *pointer)
{
    UNIMPLEMENTED();
}

// GL_OES_point_size_array
void Context::pointSizePointer(GLenum type, GLsizei stride, const void *ptr)
{
    UNIMPLEMENTED();
}

// GL_OES_query_matrix
GLbitfield Context::queryMatrixx(GLfixed *mantissa, GLint *exponent)
{
    UNIMPLEMENTED();
    return 0;
}

// GL_OES_texture_cube_map
void Context::getTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
    UNIMPLEMENTED();
}

void Context::getTexGeniv(GLenum coord, GLenum pname, GLint *params)
{
    UNIMPLEMENTED();
}

void Context::getTexGenxv(GLenum coord, GLenum pname, GLfixed *params)
{
    UNIMPLEMENTED();
}

void Context::texGenf(GLenum coord, GLenum pname, GLfloat param)
{
    UNIMPLEMENTED();
}

void Context::texGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
    UNIMPLEMENTED();
}

void Context::texGeni(GLenum coord, GLenum pname, GLint param)
{
    UNIMPLEMENTED();
}

void Context::texGeniv(GLenum coord, GLenum pname, const GLint *params)
{
    UNIMPLEMENTED();
}

void Context::texGenx(GLenum coord, GLenum pname, GLfixed param)
{
    UNIMPLEMENTED();
}

void Context::texGenxv(GLenum coord, GLenum pname, const GLint *params)
{
    UNIMPLEMENTED();
}

}  // namespace gl
