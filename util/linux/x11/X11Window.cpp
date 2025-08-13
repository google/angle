//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// X11Window.cpp: Implementation of OSWindow for X11

#include "util/linux/x11/X11Window.h"

#include "common/debug.h"
#include "util/Timer.h"
#include "util/test_utils.h"

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

namespace
{

Bool WaitForMapNotify(Display *dpy, XEvent *event, XPointer window)
{
    return event->type == MapNotify && event->xmap.window == reinterpret_cast<Window>(window);
}

static KeyType X11CodeToKey(Display *display, unsigned int scancode)
{
    int temp;
    KeySym *keySymbols;
    keySymbols = XGetKeyboardMapping(display, scancode, 1, &temp);

    KeySym keySymbol = keySymbols[0];
    XFree(keySymbols);

    switch (keySymbol)
    {
        case XK_Shift_L:
            return KeyType::LSHIFT;
        case XK_Shift_R:
            return KeyType::RSHIFT;
        case XK_Alt_L:
            return KeyType::LALT;
        case XK_Alt_R:
            return KeyType::RALT;
        case XK_Control_L:
            return KeyType::LCONTROL;
        case XK_Control_R:
            return KeyType::RCONTROL;
        case XK_Super_L:
            return KeyType::LSYSTEM;
        case XK_Super_R:
            return KeyType::RSYSTEM;
        case XK_Menu:
            return KeyType::MENU;

        case XK_semicolon:
            return KeyType::SEMICOLON;
        case XK_slash:
            return KeyType::SLASH;
        case XK_equal:
            return KeyType::EQUAL;
        case XK_minus:
            return KeyType::DASH;
        case XK_bracketleft:
            return KeyType::LBRACKET;
        case XK_bracketright:
            return KeyType::RBRACKET;
        case XK_comma:
            return KeyType::COMMA;
        case XK_period:
            return KeyType::PERIOD;
        case XK_backslash:
            return KeyType::BACKSLASH;
        case XK_asciitilde:
            return KeyType::TILDE;
        case XK_Escape:
            return KeyType::ESCAPE;
        case XK_space:
            return KeyType::SPACE;
        case XK_Return:
            return KeyType::RETURN;
        case XK_BackSpace:
            return KeyType::BACK;
        case XK_Tab:
            return KeyType::TAB;
        case XK_Page_Up:
            return KeyType::PAGEUP;
        case XK_Page_Down:
            return KeyType::PAGEDOWN;
        case XK_End:
            return KeyType::END;
        case XK_Home:
            return KeyType::HOME;
        case XK_Insert:
            return KeyType::INSERT;
        case XK_Delete:
            return KeyType::DEL;
        case XK_KP_Add:
            return KeyType::ADD;
        case XK_KP_Subtract:
            return KeyType::SUBTRACT;
        case XK_KP_Multiply:
            return KeyType::MULTIPLY;
        case XK_KP_Divide:
            return KeyType::DIVIDE;
        case XK_Pause:
            return KeyType::PAUSE;

        case XK_F1:
            return KeyType::F1;
        case XK_F2:
            return KeyType::F2;
        case XK_F3:
            return KeyType::F3;
        case XK_F4:
            return KeyType::F4;
        case XK_F5:
            return KeyType::F5;
        case XK_F6:
            return KeyType::F6;
        case XK_F7:
            return KeyType::F7;
        case XK_F8:
            return KeyType::F8;
        case XK_F9:
            return KeyType::F9;
        case XK_F10:
            return KeyType::F10;
        case XK_F11:
            return KeyType::F11;
        case XK_F12:
            return KeyType::F12;
        case XK_F13:
            return KeyType::F13;
        case XK_F14:
            return KeyType::F14;
        case XK_F15:
            return KeyType::F15;

        case XK_Left:
            return KeyType::LEFT;
        case XK_Right:
            return KeyType::RIGHT;
        case XK_Down:
            return KeyType::DOWN;
        case XK_Up:
            return KeyType::UP;

        case XK_KP_Insert:
            return KeyType::NUMPAD0;
        case XK_KP_End:
            return KeyType::NUMPAD1;
        case XK_KP_Down:
            return KeyType::NUMPAD2;
        case XK_KP_Page_Down:
            return KeyType::NUMPAD3;
        case XK_KP_Left:
            return KeyType::NUMPAD4;
        case XK_KP_5:
            return KeyType::NUMPAD5;
        case XK_KP_Right:
            return KeyType::NUMPAD6;
        case XK_KP_Home:
            return KeyType::NUMPAD7;
        case XK_KP_Up:
            return KeyType::NUMPAD8;
        case XK_KP_Page_Up:
            return KeyType::NUMPAD9;

        case XK_a:
            return KeyType::A;
        case XK_b:
            return KeyType::B;
        case XK_c:
            return KeyType::C;
        case XK_d:
            return KeyType::D;
        case XK_e:
            return KeyType::E;
        case XK_f:
            return KeyType::F;
        case XK_g:
            return KeyType::G;
        case XK_h:
            return KeyType::H;
        case XK_i:
            return KeyType::I;
        case XK_j:
            return KeyType::J;
        case XK_k:
            return KeyType::K;
        case XK_l:
            return KeyType::L;
        case XK_m:
            return KeyType::M;
        case XK_n:
            return KeyType::N;
        case XK_o:
            return KeyType::O;
        case XK_p:
            return KeyType::P;
        case XK_q:
            return KeyType::Q;
        case XK_r:
            return KeyType::R;
        case XK_s:
            return KeyType::S;
        case XK_t:
            return KeyType::T;
        case XK_u:
            return KeyType::U;
        case XK_v:
            return KeyType::V;
        case XK_w:
            return KeyType::W;
        case XK_x:
            return KeyType::X;
        case XK_y:
            return KeyType::Y;
        case XK_z:
            return KeyType::Z;

        case XK_1:
            return KeyType::NUM1;
        case XK_2:
            return KeyType::NUM2;
        case XK_3:
            return KeyType::NUM3;
        case XK_4:
            return KeyType::NUM4;
        case XK_5:
            return KeyType::NUM5;
        case XK_6:
            return KeyType::NUM6;
        case XK_7:
            return KeyType::NUM7;
        case XK_8:
            return KeyType::NUM8;
        case XK_9:
            return KeyType::NUM9;
        case XK_0:
            return KeyType::NUM0;
    }

    return KeyType(0);
}

static void AddX11KeyStateToEvent(Event *event, unsigned int state)
{
    event->Key.Shift   = state & ShiftMask;
    event->Key.Control = state & ControlMask;
    event->Key.Alt     = state & Mod1Mask;
    event->Key.System  = state & Mod4Mask;
}

void setWindowSizeHints(Display *display, Window window, int width, int height)
{
    // Set PMinSize and PMaxSize on XSizeHints so windows larger than the screen do not get adjusted
    // to screen size
    XSizeHints *sizeHints = XAllocSizeHints();
    sizeHints->flags      = PMinSize | PMaxSize;
    sizeHints->min_width  = width;
    sizeHints->min_height = height;
    sizeHints->max_width  = width;
    sizeHints->max_height = height;

    XSetWMNormalHints(display, window, sizeHints);

    XFree(sizeHints);
}

}  // namespace

class ANGLE_UTIL_EXPORT X11Window : public OSWindow
{
  public:
    X11Window();
    X11Window(int visualId);
    ~X11Window() override;

    void disableErrorMessageDialog() override;
    void destroy() override;

    void resetNativeWindow() override;
    EGLNativeWindowType getNativeWindow() const override;
    void *getPlatformExtension() override;
    EGLNativeDisplayType getNativeDisplay() const override;

    void messageLoop() override;

    void setMousePosition(int x, int y) override;
    bool setOrientation(int width, int height) override;
    bool setPosition(int x, int y) override;
    bool resize(int width, int height) override;
    void setVisible(bool isVisible) override;

    void signalTestEvent() override;

  private:
    bool initializeImpl(const std::string &name, int width, int height) override;
    void processEvent(const XEvent &event);

    Atom WM_DELETE_WINDOW;
    Atom WM_PROTOCOLS;
    Atom TEST_EVENT;

    Display *mDisplay;
    Window mWindow;
    int mRequestedVisualId;
    bool mVisible;
    bool mConfigured = false;
    bool mDestroyed  = false;
};

X11Window::X11Window()
    : WM_DELETE_WINDOW(None),
      WM_PROTOCOLS(None),
      TEST_EVENT(None),
      mDisplay(nullptr),
      mWindow(0),
      mRequestedVisualId(-1),
      mVisible(false)
{}

X11Window::X11Window(int visualId)
    : WM_DELETE_WINDOW(None),
      WM_PROTOCOLS(None),
      TEST_EVENT(None),
      mDisplay(nullptr),
      mWindow(0),
      mRequestedVisualId(visualId),
      mVisible(false)
{}

X11Window::~X11Window()
{
    destroy();
}

bool X11Window::initializeImpl(const std::string &name, int width, int height)
{
    destroy();

    mDisplay = XOpenDisplay(nullptr);
    if (!mDisplay)
    {
        return false;
    }

    {
        int screen  = DefaultScreen(mDisplay);
        Window root = RootWindow(mDisplay, screen);

        Visual *visual;
        if (mRequestedVisualId == -1)
        {
            visual = DefaultVisual(mDisplay, screen);
        }
        else
        {
            XVisualInfo visualTemplate;
            visualTemplate.visualid = mRequestedVisualId;

            int numVisuals = 0;
            XVisualInfo *visuals =
                XGetVisualInfo(mDisplay, VisualIDMask, &visualTemplate, &numVisuals);
            if (numVisuals <= 0)
            {
                return false;
            }
            ASSERT(numVisuals == 1);

            visual = visuals[0].visual;
            XFree(visuals);
        }

        int depth         = DefaultDepth(mDisplay, screen);
        Colormap colormap = XCreateColormap(mDisplay, root, visual, AllocNone);

        XSetWindowAttributes attributes;
        unsigned long attributeMask = CWBorderPixel | CWColormap | CWEventMask;

        attributes.event_mask = StructureNotifyMask | PointerMotionMask | ButtonPressMask |
                                ButtonReleaseMask | FocusChangeMask | EnterWindowMask |
                                LeaveWindowMask | KeyPressMask | KeyReleaseMask;
        attributes.border_pixel = 0;
        attributes.colormap     = colormap;

        mWindow = XCreateWindow(mDisplay, root, 0, 0, width, height, 0, depth, InputOutput, visual,
                                attributeMask, &attributes);
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
    WM_PROTOCOLS     = XInternAtom(mDisplay, "WM_PROTOCOLS", False);
    if (WM_DELETE_WINDOW == None || WM_PROTOCOLS == None)
    {
        destroy();
        return false;
    }

    if (XSetWMProtocols(mDisplay, mWindow, &WM_DELETE_WINDOW, 1) == 0)
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

    setWindowSizeHints(mDisplay, mWindow, width, height);

    XFlush(mDisplay);

    mX      = 0;
    mY      = 0;
    mWidth  = width;
    mHeight = height;

    return true;
}

void X11Window::disableErrorMessageDialog() {}

void X11Window::destroy()
{
    if (mWindow)
    {
        XDestroyWindow(mDisplay, mWindow);
        XFlush(mDisplay);
        // There appears to be a race condition where XDestroyWindow+XCreateWindow ignores
        // the new size (the same window normally gets reused but this only happens sometimes on
        // some X11 versions). Wait until we get the destroy notification.
        mWindow = 0;  // Set before messageLoop() to avoid a race in processEvent().
        while (!mDestroyed)
        {
            messageLoop();
            angle::Sleep(10);
        }
    }
    if (mDisplay)
    {
        XCloseDisplay(mDisplay);
        mDisplay = nullptr;
    }
    WM_DELETE_WINDOW = None;
    WM_PROTOCOLS     = None;
}

void X11Window::resetNativeWindow() {}

EGLNativeWindowType X11Window::getNativeWindow() const
{
    return mWindow;
}

void *X11Window::getPlatformExtension()
{
    // X11 native window for eglCreateSurfacePlatformEXT is Window*
    return &mWindow;
}

EGLNativeDisplayType X11Window::getNativeDisplay() const
{
    return reinterpret_cast<EGLNativeDisplayType>(mDisplay);
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

bool X11Window::setOrientation(int width, int height)
{
    UNIMPLEMENTED();
    return false;
}

bool X11Window::setPosition(int x, int y)
{
    XMoveWindow(mDisplay, mWindow, x, y);
    XFlush(mDisplay);
    return true;
}

bool X11Window::resize(int width, int height)
{
    setWindowSizeHints(mDisplay, mWindow, width, height);
    XResizeWindow(mDisplay, mWindow, width, height);

    XFlush(mDisplay);

    Timer timer;
    timer.start();

    // Wait until the window has actually been resized so that the code calling resize
    // can assume the window has been resized.
    const double kResizeWaitDelay = 0.2;
    while ((mHeight != height || mWidth != width) &&
           timer.getElapsedWallClockTime() < kResizeWaitDelay)
    {
        messageLoop();
        angle::Sleep(10);
    }

    return true;
}

void X11Window::setVisible(bool isVisible)
{
    if (mVisible == isVisible)
    {
        return;
    }

    if (isVisible)
    {
        XMapWindow(mDisplay, mWindow);

        // Wait until we get an event saying this window is mapped so that the
        // code calling setVisible can assume the window is visible.
        // This is important when creating a framebuffer as the framebuffer content
        // is undefined when the window is not visible.
        XEvent placeholderEvent;
        XIfEvent(mDisplay, &placeholderEvent, WaitForMapNotify,
                 reinterpret_cast<XPointer>(mWindow));

        // Block until we get ConfigureNotify to set up fully before returning.
        mConfigured = false;
        while (!mConfigured)
        {
            messageLoop();
            angle::Sleep(10);
        }
    }
    else
    {
        XUnmapWindow(mDisplay, mWindow);
        XFlush(mDisplay);
    }

    mVisible = isVisible;
}

void X11Window::signalTestEvent()
{
    XEvent event;
    event.type                 = ClientMessage;
    event.xclient.message_type = TEST_EVENT;
    // Format needs to be valid or a BadValue is generated
    event.xclient.format = 32;

    // Hijack StructureNotifyMask as we know we will be listening for it.
    XSendEvent(mDisplay, mWindow, False, StructureNotifyMask, &event);

    // For test events, the tests want to check that it really did arrive, and they don't wait
    // long.  XSync here makes sure the event is sent by the time the messageLoop() is called.
    XSync(mDisplay, false);
}

void X11Window::processEvent(const XEvent &xEvent)
{
    // TODO(cwallez) text events
    switch (xEvent.type)
    {
        case ButtonPress:
        {
            Event event;
            MouseButtonType button = MouseButtonType::UNKNOWN;
            int wheelY             = 0;

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
                    break;
                case 7:
                    break;

                case Button1:
                    button = MouseButtonType::LEFT;
                    break;
                case Button2:
                    button = MouseButtonType::MIDDLE;
                    break;
                case Button3:
                    button = MouseButtonType::RIGHT;
                    break;
                case 8:
                    button = MouseButtonType::BUTTON4;
                    break;
                case 9:
                    button = MouseButtonType::BUTTON5;
                    break;

                default:
                    break;
            }

            if (wheelY != 0)
            {
                event.Type             = Event::EVENT_MOUSE_WHEEL_MOVED;
                event.MouseWheel.Delta = wheelY;
                pushEvent(event);
            }

            if (button != MouseButtonType::UNKNOWN)
            {
                event.Type               = Event::EVENT_MOUSE_BUTTON_RELEASED;
                event.MouseButton.Button = button;
                event.MouseButton.X      = xEvent.xbutton.x;
                event.MouseButton.Y      = xEvent.xbutton.y;
                pushEvent(event);
            }
        }
        break;

        case ButtonRelease:
        {
            Event event;
            MouseButtonType button = MouseButtonType::UNKNOWN;

            switch (xEvent.xbutton.button)
            {
                case Button1:
                    button = MouseButtonType::LEFT;
                    break;
                case Button2:
                    button = MouseButtonType::MIDDLE;
                    break;
                case Button3:
                    button = MouseButtonType::RIGHT;
                    break;
                case 8:
                    button = MouseButtonType::BUTTON4;
                    break;
                case 9:
                    button = MouseButtonType::BUTTON5;
                    break;

                default:
                    break;
            }

            if (button != MouseButtonType::UNKNOWN)
            {
                event.Type               = Event::EVENT_MOUSE_BUTTON_RELEASED;
                event.MouseButton.Button = button;
                event.MouseButton.X      = xEvent.xbutton.x;
                event.MouseButton.Y      = xEvent.xbutton.y;
                pushEvent(event);
            }
        }
        break;

        case KeyPress:
        {
            Event event;
            event.Type     = Event::EVENT_KEY_PRESSED;
            event.Key.Code = X11CodeToKey(mDisplay, xEvent.xkey.keycode);
            AddX11KeyStateToEvent(&event, xEvent.xkey.state);
            pushEvent(event);
        }
        break;

        case KeyRelease:
        {
            Event event;
            event.Type     = Event::EVENT_KEY_RELEASED;
            event.Key.Code = X11CodeToKey(mDisplay, xEvent.xkey.keycode);
            AddX11KeyStateToEvent(&event, xEvent.xkey.state);
            pushEvent(event);
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
            event.Type        = Event::EVENT_MOUSE_MOVED;
            event.MouseMove.X = xEvent.xmotion.x;
            event.MouseMove.Y = xEvent.xmotion.y;
            pushEvent(event);
        }
        break;

        case ConfigureNotify:
        {
            mConfigured = true;
            if (mWindow == 0)
            {
                break;
            }
            if (xEvent.xconfigure.width != mWidth || xEvent.xconfigure.height != mHeight)
            {
                Event event;
                event.Type        = Event::EVENT_RESIZED;
                event.Size.Width  = xEvent.xconfigure.width;
                event.Size.Height = xEvent.xconfigure.height;
                pushEvent(event);
            }
            if (xEvent.xconfigure.x != mX || xEvent.xconfigure.y != mY)
            {
                // Sometimes, the window manager reparents our window (for example
                // when resizing) then the X and Y coordinates will be with respect to
                // the new parent and not what the user wants to know. Use
                // XTranslateCoordinates to get the coordinates on the screen.
                int screen  = DefaultScreen(mDisplay);
                Window root = RootWindow(mDisplay, screen);

                int x, y;
                Window child;
                XTranslateCoordinates(mDisplay, mWindow, root, 0, 0, &x, &y, &child);

                if (x != mX || y != mY)
                {
                    Event event;
                    event.Type   = Event::EVENT_MOVED;
                    event.Move.X = x;
                    event.Move.Y = y;
                    pushEvent(event);
                }
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
            // Note: we already received WM_DELETE_WINDOW
            mDestroyed = true;
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

OSWindow *CreateX11Window()
{
    return new X11Window();
}

OSWindow *CreateX11WindowWithVisualId(int visualId)
{
    return new X11Window(visualId);
}

bool IsX11WindowAvailable()
{
    Display *display = XOpenDisplay(nullptr);
    if (!display)
    {
        return false;
    }
    XCloseDisplay(display);
    return true;
}
