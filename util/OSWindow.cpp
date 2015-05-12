//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "OSWindow.h"

#include <iostream>

#include "common/debug.h"

#ifndef DEBUG_EVENTS
#define DEBUG_EVENTS 0
#endif

#if DEBUG_EVENTS
static const char *MouseButtonName(MouseButton button)
{
    switch (button)
    {
      case MOUSEBUTTON_UNKNOWN:
        return "Unknown";
      case MOUSEBUTTON_LEFT:
        return "Left";
      case MOUSEBUTTON_RIGHT:
        return "Right";
      case MOUSEBUTTON_MIDDLE:
        return "Middle";
      case MOUSEBUTTON_BUTTON4:
        return "Button4";
      case MOUSEBUTTON_BUTTON5:
        return "Button5";
      default:
        UNREACHABLE();
        return nullptr;
    }
}

static void PrintEvent(const Event& event)
{
    switch (event.Type)
    {
      case Event::EVENT_CLOSED:
        std::cout << "Event: Window Closed" << std::endl;
        break;
      case Event::EVENT_MOVED:
        std::cout << "Event: Window Moved (" << event.Move.X
                  << ", " << event.Move.Y << ")" << std::endl;
        break;
      case Event::EVENT_RESIZED:
        std::cout << "Event: Window Resized (" << event.Size.Width
                  << ", " << event.Size.Height << ")" << std::endl;
        break;
      case Event::EVENT_LOST_FOCUS:
        std::cout << "Event: Window Lost Focus" << std::endl;
        break;
      case Event::EVENT_GAINED_FOCUS:
        std::cout << "Event: Window Gained Focus" << std::endl;
        break;
      case Event::TEXT_ENTERED:
        // TODO(cwallez) show the character
        std::cout << "Event: Text Entered" << std::endl;
        break;
      case Event::EVENT_KEY_PRESSED:
        // TODO(cwallez) show the key
        std::cout << "Event: Key Pressed" << std::endl;
        break;
      case Event::EVENT_KEY_RELEASED:
        // TODO(cwallez) show the key
        std::cout << "Event: Key Released" << std::endl;
        break;
      case Event::EVENT_MOUSE_WHEEL_MOVED:
        std::cout << "Event: Mouse Wheel (" << event.MouseWheel.Delta << ")" << std::endl;
        break;
      case Event::EVENT_MOUSE_BUTTON_PRESSED:
        std::cout << "Event: Mouse Button Pressed " << MouseButtonName(event.MouseButton.Button) <<
                  " at (" << event.MouseButton.X << ", " << event.MouseButton.Y << ")" << std::endl;
        break;
      case Event::EVENT_MOUSE_BUTTON_RELEASED:
        std::cout << "Event: Mouse Button Released " << MouseButtonName(event.MouseButton.Button) <<
                  " at (" << event.MouseButton.X << ", " << event.MouseButton.Y << ")" << std::endl;
        break;
      case Event::EVENT_MOUSE_MOVED:
        std::cout << "Event: Mouse Moved (" << event.MouseMove.X
                  << ", " << event.MouseMove.Y << ")" << std::endl;
        break;
      case Event::EVENT_MOUSE_ENTERED:
        std::cout << "Event: Mouse Entered Window" << std::endl;
        break;
      case Event::EVENT_MOUSE_LEFT:
        std::cout << "Event: Mouse Left Window" << std::endl;
        break;
      case Event::EVENT_TEST:
        std::cout << "Event: Test" << std::endl;
        break;
      default:
        UNREACHABLE();
        break;
    }
}
#endif

OSWindow::OSWindow()
    : mX(0),
      mY(0),
      mWidth(0),
      mHeight(0)
{
}

OSWindow::~OSWindow()
{}

int OSWindow::getX() const
{
    return mX;
}

int OSWindow::getY() const
{
    return mY;
}

int OSWindow::getWidth() const
{
    return mWidth;
}

int OSWindow::getHeight() const
{
    return mHeight;
}

bool OSWindow::popEvent(Event *event)
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

void OSWindow::pushEvent(Event event)
{
    switch (event.Type)
    {
      case Event::EVENT_MOVED:
        mX = event.Move.X;
        mY = event.Move.Y;
        break;
      case Event::EVENT_RESIZED:
        mWidth = event.Size.Width;
        mHeight = event.Size.Height;
        break;
      default:
        break;
    }

    mEvents.push_back(event);

#if DEBUG_EVENTS
    PrintEvent(event);
#endif
}

bool OSWindow::didTestEventFire()
{
    Event topEvent;
    while (popEvent(&topEvent))
    {
        if (topEvent.Type == Event::EVENT_TEST)
        {
            return true;
        }
    }

    return false;
}
