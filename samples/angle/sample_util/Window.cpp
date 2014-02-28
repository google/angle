//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "Window.h"

Window::Window()
    : mWidth(0),
      mHeight(0)
{
}

int Window::getWidth() const
{
    return mWidth;
}

int Window::getHeight() const
{
    return mHeight;
}

bool Window::popEvent(Event *event)
{
    if (mEvents.size() > 0 && event)
    {
        *event = mEvents.front();
        mEvents.pop_front();
        return true;
    }
    else
    {
        return false;
    }
}

void Window::pushEvent(Event event)
{
    switch (event.Type)
    {
      case Event::EVENT_RESIZED:
        mWidth = event.Size.Width;
        mHeight = event.Size.Height;
        break;
    }

    mEvents.push_back(event);
}
