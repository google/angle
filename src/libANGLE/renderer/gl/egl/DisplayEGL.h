//
// Copyright (c) 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DisplayEGL.h: Common across EGL parts of platform specific egl::Display implementations

#ifndef LIBANGLE_RENDERER_GL_EGL_DISPLAYEGL_H_
#define LIBANGLE_RENDERER_GL_EGL_DISPLAYEGL_H_

#include "libANGLE/renderer/gl/DisplayGL.h"
#include "libANGLE/renderer/gl/egl/FunctionsEGL.h"

namespace rx
{

class DisplayEGL : public DisplayGL
{
  public:
    DisplayEGL(const egl::DisplayState &state);
    ~DisplayEGL() override;

    ImageImpl *createImage(const egl::ImageState &state,
                           const gl::Context *context,
                           EGLenum target,
                           const egl::AttributeMap &attribs) override;

    std::string getVendorString() const override;

    virtual void destroyNativeContext(EGLContext context) = 0;

  protected:
    egl::Error initializeContext(EGLContext shareContext,
                                 const egl::AttributeMap &eglAttributes,
                                 EGLContext *outContext) const;

    void generateExtensions(egl::DisplayExtensions *outExtensions) const override;

    FunctionsEGL *mEGL;
    EGLConfig mConfig;

  private:
    void generateCaps(egl::Caps *outCaps) const override;
};

}  // namespace rx

#endif /* LIBANGLE_RENDERER_GL_EGL_DISPLAYEGL_H_ */
