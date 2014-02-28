//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef SAMPLE_UTIL_SAMPLE_APPLICATION_H
#define SAMPLE_UTIL_SAMPLE_APPLICATION_H

#define GL_GLEXT_PROTOTYPES

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "Window.h"
#include "Timer.h"

#include <string>
#include <list>
#include <cstdint>
#include <memory>

class SampleApplication
{
  public:
    SampleApplication(const std::string& name, size_t width, size_t height);
    virtual ~SampleApplication();

    virtual bool initialize();
    virtual void destroy();

    virtual void step(float dt, double totalTime);
    virtual void draw();

    Window *getWindow() const;

    bool popEvent(Event *event);

    int run();
    void exit();

  private:
    bool initializeGL();
    void destroyGL();

    EGLConfig mConfig;
    EGLSurface mSurface;
    EGLContext mContext;

    GLuint mClientVersion;
    size_t mWidth;
    size_t mHeight;
    std::string mName;

    bool mRunning;

    std::unique_ptr<Timer> mTimer;
    std::unique_ptr<Window> mWindow;
};

#endif // SAMPLE_UTIL_SAMPLE_APPLICATION_H
