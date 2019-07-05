//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// capture_gles1_params.cpp:
//   Pointer parameter capture functions for the OpenGL ES 1.0 entry points.

#include "libANGLE/capture_gles_1_0_autogen.h"

using namespace angle;

namespace gl
{

void CaptureClipPlanef_eqn(Context *context,
                           GLenum p,
                           const GLfloat *eqn,
                           bool isCallValid,
                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureClipPlanex_equation(Context *context,
                                GLenum plane,
                                const GLfixed *equation,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureColorPointer_pointer(Context *context,
                                 GLint size,
                                 VertexAttribType typePacked,
                                 GLsizei stride,
                                 const void *pointer,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureFogfv_params(Context *context,
                         GLenum pname,
                         const GLfloat *params,
                         bool isCallValid,
                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureFogxv_param(Context *context,
                        GLenum pname,
                        const GLfixed *param,
                        bool isCallValid,
                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetClipPlanef_equation(Context *context,
                                   GLenum plane,
                                   GLfloat *equation,
                                   bool isCallValid,
                                   ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetClipPlanex_equation(Context *context,
                                   GLenum plane,
                                   GLfixed *equation,
                                   bool isCallValid,
                                   ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetFixedv_params(Context *context,
                             GLenum pname,
                             GLfixed *params,
                             bool isCallValid,
                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetLightfv_params(Context *context,
                              GLenum light,
                              LightParameter pnamePacked,
                              GLfloat *params,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetLightxv_params(Context *context,
                              GLenum light,
                              LightParameter pnamePacked,
                              GLfixed *params,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetMaterialfv_params(Context *context,
                                 GLenum face,
                                 MaterialParameter pnamePacked,
                                 GLfloat *params,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetMaterialxv_params(Context *context,
                                 GLenum face,
                                 MaterialParameter pnamePacked,
                                 GLfixed *params,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetPointerv_params(Context *context,
                               GLenum pname,
                               void **params,
                               bool isCallValid,
                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexEnvfv_params(Context *context,
                               TextureEnvTarget targetPacked,
                               TextureEnvParameter pnamePacked,
                               GLfloat *params,
                               bool isCallValid,
                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexEnviv_params(Context *context,
                               TextureEnvTarget targetPacked,
                               TextureEnvParameter pnamePacked,
                               GLint *params,
                               bool isCallValid,
                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexEnvxv_params(Context *context,
                               TextureEnvTarget targetPacked,
                               TextureEnvParameter pnamePacked,
                               GLfixed *params,
                               bool isCallValid,
                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexParameterxv_params(Context *context,
                                     TextureType targetPacked,
                                     GLenum pname,
                                     GLfixed *params,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureLightModelfv_params(Context *context,
                                GLenum pname,
                                const GLfloat *params,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureLightModelxv_param(Context *context,
                               GLenum pname,
                               const GLfixed *param,
                               bool isCallValid,
                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureLightfv_params(Context *context,
                           GLenum light,
                           LightParameter pnamePacked,
                           const GLfloat *params,
                           bool isCallValid,
                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureLightxv_params(Context *context,
                           GLenum light,
                           LightParameter pnamePacked,
                           const GLfixed *params,
                           bool isCallValid,
                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureLoadMatrixf_m(Context *context,
                          const GLfloat *m,
                          bool isCallValid,
                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureLoadMatrixx_m(Context *context,
                          const GLfixed *m,
                          bool isCallValid,
                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMaterialfv_params(Context *context,
                              GLenum face,
                              MaterialParameter pnamePacked,
                              const GLfloat *params,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMaterialxv_param(Context *context,
                             GLenum face,
                             MaterialParameter pnamePacked,
                             const GLfixed *param,
                             bool isCallValid,
                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMultMatrixf_m(Context *context,
                          const GLfloat *m,
                          bool isCallValid,
                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMultMatrixx_m(Context *context,
                          const GLfixed *m,
                          bool isCallValid,
                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureNormalPointer_pointer(Context *context,
                                  VertexAttribType typePacked,
                                  GLsizei stride,
                                  const void *pointer,
                                  bool isCallValid,
                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CapturePointParameterfv_params(Context *context,
                                    PointParameter pnamePacked,
                                    const GLfloat *params,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CapturePointParameterxv_params(Context *context,
                                    PointParameter pnamePacked,
                                    const GLfixed *params,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexCoordPointer_pointer(Context *context,
                                    GLint size,
                                    VertexAttribType typePacked,
                                    GLsizei stride,
                                    const void *pointer,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexEnvfv_params(Context *context,
                            TextureEnvTarget targetPacked,
                            TextureEnvParameter pnamePacked,
                            const GLfloat *params,
                            bool isCallValid,
                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexEnviv_params(Context *context,
                            TextureEnvTarget targetPacked,
                            TextureEnvParameter pnamePacked,
                            const GLint *params,
                            bool isCallValid,
                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexEnvxv_params(Context *context,
                            TextureEnvTarget targetPacked,
                            TextureEnvParameter pnamePacked,
                            const GLfixed *params,
                            bool isCallValid,
                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexParameterxv_params(Context *context,
                                  TextureType targetPacked,
                                  GLenum pname,
                                  const GLfixed *params,
                                  bool isCallValid,
                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureVertexPointer_pointer(Context *context,
                                  GLint size,
                                  VertexAttribType typePacked,
                                  GLsizei stride,
                                  const void *pointer,
                                  bool isCallValid,
                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

}  // namespace gl
