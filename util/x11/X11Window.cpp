//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// X11Window.cpp: Implementation of OSWindow for X11

#include "x11/X11Window.h"

namespace {

Bool WaitForMapNotify(Display *dpy, XEvent *event, XPointer window)
{
    return event->type == MapNotify && event->xmap.window == reinterpret_cast<Window>(window);
}

}

X11Window::X11Window()
    : WM_DELETE_WINDOW(None),
      mDisplay(nullptr),
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

    {
        int screen = DefaultScreen(mDisplay);
        Window root = RootWindow(mDisplay, screen);

        Colormap colormap = XCreateColormap(mDisplay, root, DefaultVisual(mDisplay, screen), AllocNone);
        int depth = DefaultDepth(mDisplay, screen);
        Visual *visual = DefaultVisual(mDisplay, screen);

        XSetWindowAttributes attributes;
        unsigned long attributeMask = CWBorderPixel | CWColormap | CWEventMask;

        attributes.event_mask = StructureNotifyMask | PointerMotionMask | ButtonPressMask |
                                ButtonReleaseMask | FocusChangeMask | EnterWindowMask |
                                LeaveWindowMask;
        attributes.border_pixel = 0;
        attributes.colormap = colormap;

        mWindow = XCreateWindow(mDisplay, root, 0, 0, width, height, 0, depth, InputOutput,
                                visual, attributeMask, &attributes);
        XFreeColormap(mDisplay, colormap);
    }

    if (!mWindow)
    {
        destroy();
        return false;
    }

    // Tell the window manager to notify us when the user wants to close the
    // window so we can do it ourselves.
    WM_DELETE_WINDOW = XInternAtom(mDisplay, "WM_DELETE_WINDOW", False);
    WM_PROTOCOLS = XInternAtom(mDisplay, "WM_PROTOCOLS", False);
    if (WM_DELETE_WINDOW == None || WM_PROTOCOLS == None)
    {
        destroy();
        return false;
    }

    if(XSetWMProtocols(mDisplay, mWindow, &WM_DELETE_WINDOW, 1) == 0)
    {
        destroy();
        return false;
    }

    // Create an atom to identify our test event
    TEST_EVENT = XInternAtom(mDisplay, "ANGLE_TEST_EVENT", False);
    if (TEST_EVENT == None)
    {
        destroy();
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
    WM_DELETE_WINDOW = None;
    WM_PROTOCOLS = None;
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
    int eventCount = XPending(mDisplay);
    while (eventCount--)
    {
        XEvent event;
        XNextEvent(mDisplay, &event);
        processEvent(event);
    }
}

void X11Window::setMousePosition(int x, int y)
{
    XWarpPointer(mDisplay, None, mWindow, 0, 0, 0, 0, x, y);
}

OSWindow *CreateOSWindow()
{
    return new X11Window();
}

bool X11Window::setPosition(int x, int y)
{
    XMoveWindow(mDisplay, mWindow, x, y);
    XFlush(mDisplay);
    return true;
}

bool X11Window::resize(int width, int height)
{
    XResizeWindow(mDisplay, mWindow, width, height);
    XFlush(mDisplay);
    return true;
}

void X11Window::setVisible(bool isVisible)
{
    if (isVisible)
    {
        XMapWindow(mDisplay, mWindow);

        // Wait until we get an event saying this window is mapped so that the
        // code calling setVisible can assume the window is visible.
        // This is important when creating a framebuffer as the framebuffer content
        // is undefined when the window is not visible.
        XEvent dummyEvent;
        XIfEvent(mDisplay, &dummyEvent, WaitForMapNotify, reinterpret_cast<XPointer>(mWindow));
    }
    else
    {
        XUnmapWindow(mDisplay, mWindow);
        XFlush(mDisplay);
    }
}

void X11Window::signalTestEvent()
{
    XEvent event;
    event.type = ClientMessage;
    event.xclient.message_type = TEST_EVENT;
    // Format needs to be valid or a BadValue is generated
    event.xclient.format = 32;

    // Hijack StructureNotifyMask as we know we will be listening for it.
    XSendEvent(mDisplay, mWindow, False, StructureNotifyMask, &event);
}

void X11Window::processEvent(const XEvent &xEvent)
{
    // TODO(cwallez) handle key presses and text events
    switch (xEvent.type)
    {
      case ButtonPress:
        {
            Event event;
            MouseButton button = MOUSEBUTTON_UNKNOWN;
            int wheelX = 0;
            int wheelY = 0;

            // The mouse wheel updates are sent via button events.
            switch (xEvent.xbutton.button)
            {
              case Button4:
                wheelY = 1;
                break;
              case Button5:
                wheelY = -1;
                break;
              case 6:
                wheelX = 1;
                break;
              case 7:
                wheelX = -1;
                break;

              case Button1:
                button = MOUSEBUTTON_LEFT;
                break;
              case Button2:
                button = MOUSEBUTTON_MIDDLE;
                break;
              case Button3:
                button = MOUSEBUTTON_RIGHT;
                break;
              case 8:
                button = MOUSEBUTTON_BUTTON4;
                break;
              case 9:
                button = MOUSEBUTTON_BUTTON5;
                break;

              default:
                break;
            }

            if (wheelY != 0)
            {
                event.Type = Event::EVENT_MOUSE_WHEEL_MOVED;
                event.MouseWheel.Delta = wheelY;
                pushEvent(event);
            }

            if (button != MOUSEBUTTON_UNKNOWN)
            {
                event.Type = Event::EVENT_MOUSE_BUTTON_RELEASED;
                event.MouseButton.Button = button;
                event.MouseButton.X = xEvent.xbutton.x;
                event.MouseButton.Y = xEvent.xbutton.y;
                pushEvent(event);
            }
        }
        break;

      case ButtonRelease:
        {
            Event event;
            MouseButton button = MOUSEBUTTON_UNKNOWN;

            switch (xEvent.xbutton.button)
            {
              case Button1:
                button = MOUSEBUTTON_LEFT;
                break;
              case Button2:
                button = MOUSEBUTTON_MIDDLE;
                break;
              case Button3:
                button = MOUSEBUTTON_RIGHT;
                break;
              case 8:
                button = MOUSEBUTTON_BUTTON4;
                break;
              case 9:
                button = MOUSEBUTTON_BUTTON5;
                break;

              default:
                break;
            }

            if (button != MOUSEBUTTON_UNKNOWN)
            {
                event.Type = Event::EVENT_MOUSE_BUTTON_RELEASED;
                event.MouseButton.Button = button;
                event.MouseButton.X = xEvent.xbutton.x;
                event.MouseButton.Y = xEvent.xbutton.y;
                pushEvent(event);
            }
        }
        break;

      case EnterNotify:
        {
            Event event;
            event.Type = Event::EVENT_MOUSE_ENTERED;
            pushEvent(event);
        }
        break;

      case LeaveNotify:
        {
            Event event;
            event.Type = Event::EVENT_MOUSE_LEFT;
            pushEvent(event);
        }
        break;

      case MotionNotify:
        {
            Event event;
            event.Type = Event::EVENT_MOUSE_MOVED;
            event.MouseMove.X = xEvent.xmotion.x;
            event.MouseMove.Y = xEvent.xmotion.y;
            pushEvent(event);
        }
        break;

      case ConfigureNotify:
        {
            if (xEvent.xconfigure.width != mWidth || xEvent.xconfigure.height != mHeight)
            {
                Event event;
                event.Type = Event::EVENT_RESIZED;
                event.Size.Width = xEvent.xconfigure.width;
                event.Size.Height = xEvent.xconfigure.height;
                pushEvent(event);
            }
            if (xEvent.xconfigure.x != mX || xEvent.xconfigure.y != mY)
            {
                Event event;
                event.Type = Event::EVENT_MOVED;
                event.Move.X = xEvent.xconfigure.x;
                event.Move.Y = xEvent.xconfigure.y;
                pushEvent(event);
            }
        }
        break;

      case FocusIn:
        if (xEvent.xfocus.mode == NotifyNormal || xEvent.xfocus.mode == NotifyWhileGrabbed)
        {
            Event event;
            event.Type = Event::EVENT_GAINED_FOCUS;
            pushEvent(event);
        }
        break;

      case FocusOut:
        if (xEvent.xfocus.mode == NotifyNormal || xEvent.xfocus.mode == NotifyWhileGrabbed)
        {
            Event event;
            event.Type = Event::EVENT_LOST_FOCUS;
            pushEvent(event);
        }
        break;

      case DestroyNotify:
        // We already received WM_DELETE_WINDOW
        break;

      case ClientMessage:
        if (xEvent.xclient.message_type == WM_PROTOCOLS &&
            static_cast<Atom>(xEvent.xclient.data.l[0]) == WM_DELETE_WINDOW)
        {
            Event event;
            event.Type = Event::EVENT_CLOSED;
            pushEvent(event);
        }
        else if (xEvent.xclient.message_type == TEST_EVENT)
        {
            Event event;
            event.Type = Event::EVENT_TEST;
            pushEvent(event);
        }
        break;
    }
}
