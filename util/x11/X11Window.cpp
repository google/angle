//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// X11Window.cpp: Implementation of OSWindow for X11

#include "x11/X11Window.h"

X11Window::X11Window()
    : mDisplay(nullptr),
      mWindow(0)
{
}

X11Window::~X11Window()
{
    destroy();
}

bool X11Window::initialize(const std::string &name, size_t width, size_t height)
{
    destroy();

    mDisplay = XOpenDisplay(NULL);
    if (!mDisplay)
    {
        return false;
    }

    int screen = DefaultScreen(mDisplay);
    Window root = RootWindow(mDisplay, screen);
    Colormap colormap = XCreateColormap(mDisplay, root, DefaultVisual(mDisplay, screen), AllocNone);
    int depth = DefaultDepth(mDisplay, screen);
    Visual *visual = DefaultVisual(mDisplay, screen);

    XSetWindowAttributes attributes;
    unsigned long attributeMask = CWBorderPixel | CWColormap | CWEventMask;

    // TODO(cwallez) change when input is implemented
    attributes.event_mask = 0;
    attributes.border_pixel = 0;
    attributes.colormap = colormap;

    mWindow = XCreateWindow(mDisplay, root, 0, 0, width, height, 0, depth, InputOutput,
                            visual, attributeMask, &attributes);

    if (!mWindow)
    {
        XFreeColormap(mDisplay, colormap);
        return false;
    }

    XFlush(mDisplay);

    mX = 0;
    mY = 0;
    mWidth = width;
    mHeight = height;

    return true;
}

void X11Window::destroy()
{
    if (mWindow)
    {
        XDestroyWindow(mDisplay, mWindow);
        mWindow = 0;
    }
    if (mDisplay)
    {
        XCloseDisplay(mDisplay);
        mDisplay = nullptr;
    }
}

EGLNativeWindowType X11Window::getNativeWindow() const
{
    return mWindow;
}

EGLNativeDisplayType X11Window::getNativeDisplay() const
{
    return mDisplay;
}

void X11Window::messageLoop()
{
    //TODO
}

void X11Window::setMousePosition(int x, int y)
{
    //TODO
}

OSWindow *CreateOSWindow()
{
    return new X11Window();
}

bool X11Window::setPosition(int x, int y)
{
    //TODO
    return true;
}

bool X11Window::resize(int width, int height)
{
    //TODO
    return true;
}

void X11Window::setVisible(bool isVisible)
{
    if (isVisible)
    {
        XMapWindow(mDisplay, mWindow);
    }
    else
    {
        XUnmapWindow(mDisplay, mWindow);
    }
    XFlush(mDisplay);
}

void X11Window::pushEvent(Event event)
{
    //TODO
}

void X11Window::signalTestEvent()
{
    //TODO
}
