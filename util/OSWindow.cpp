//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "OSWindow.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include "common/debug.h"
#include "common/system_utils.h"

#if defined(ANGLE_PLATFORM_ANDROID)
#    include "util/android/AndroidWindow.h"
#endif  // defined(ANGLE_PLATFORM_ANDROID)

#ifndef DEBUG_EVENTS
#    define DEBUG_EVENTS 0
#endif

#if DEBUG_EVENTS
static const char *MouseButtonName(MouseButtonType button)
{
    switch (button)
    {
        case MouseButtonType::UNKNOWN:
            return "Unknown";
        case MouseButtonType::LEFT:
            return "Left";
        case MouseButtonType::RIGHT:
            return "Right";
        case MouseButtonType::MIDDLE:
            return "Middle";
        case MouseButtonType::BUTTON4:
            return "Button4";
        case MouseButtonType::BUTTON5:
            return "Button5";
        default:
            UNREACHABLE();
            return nullptr;
    }
}

static const char *KeyName(KeyType key)
{
    switch (key)
    {
        case KeyType::UNKNOWN:
            return "Unknown";
        case KeyType::A:
            return "A";
        case KeyType::B:
            return "B";
        case KeyType::C:
            return "C";
        case KeyType::D:
            return "D";
        case KeyType::E:
            return "E";
        case KeyType::F:
            return "F";
        case KeyType::G:
            return "G";
        case KeyType::H:
            return "H";
        case KeyType::I:
            return "I";
        case KeyType::J:
            return "J";
        case KeyType::K:
            return "K";
        case KeyType::L:
            return "L";
        case KeyType::M:
            return "M";
        case KeyType::N:
            return "N";
        case KeyType::O:
            return "O";
        case KeyType::P:
            return "P";
        case KeyType::Q:
            return "Q";
        case KeyType::R:
            return "R";
        case KeyType::S:
            return "S";
        case KeyType::T:
            return "T";
        case KeyType::U:
            return "U";
        case KeyType::V:
            return "V";
        case KeyType::W:
            return "W";
        case KeyType::X:
            return "X";
        case KeyType::Y:
            return "Y";
        case KeyType::Z:
            return "Z";
        case KeyType::NUM0:
            return "Num0";
        case KeyType::NUM1:
            return "Num1";
        case KeyType::NUM2:
            return "Num2";
        case KeyType::NUM3:
            return "Num3";
        case KeyType::NUM4:
            return "Num4";
        case KeyType::NUM5:
            return "Num5";
        case KeyType::NUM6:
            return "Num6";
        case KeyType::NUM7:
            return "Num7";
        case KeyType::NUM8:
            return "Num8";
        case KeyType::NUM9:
            return "Num9";
        case KeyType::ESCAPE:
            return "Escape";
        case KeyType::LCONTROL:
            return "Left Control";
        case KeyType::LSHIFT:
            return "Left Shift";
        case KeyType::LALT:
            return "Left Alt";
        case KeyType::LSYSTEM:
            return "Left System";
        case KeyType::RCONTROL:
            return "Right Control";
        case KeyType::RSHIFT:
            return "Right Shift";
        case KeyType::RALT:
            return "Right Alt";
        case KeyType::RSYSTEM:
            return "Right System";
        case KeyType::MENU:
            return "Menu";
        case KeyType::LBRACKET:
            return "Left Bracket";
        case KeyType::RBRACKET:
            return "Right Bracket";
        case KeyType::SEMICOLON:
            return "Semicolon";
        case KeyType::COMMA:
            return "Comma";
        case KeyType::PERIOD:
            return "Period";
        case KeyType::QUOTE:
            return "Quote";
        case KeyType::SLASH:
            return "Slash";
        case KeyType::BACKSLASH:
            return "Backslash";
        case KeyType::TILDE:
            return "Tilde";
        case KeyType::EQUAL:
            return "Equal";
        case KeyType::DASH:
            return "Dash";
        case KeyType::SPACE:
            return "Space";
        case KeyType::RETURN:
            return "Return";
        case KeyType::BACK:
            return "Back";
        case KeyType::TAB:
            return "Tab";
        case KeyType::PAGEUP:
            return "Page Up";
        case KeyType::PAGEDOWN:
            return "Page Down";
        case KeyType::END:
            return "End";
        case KeyType::HOME:
            return "Home";
        case KeyType::INSERT:
            return "Insert";
        case KeyType::DEL:
            return "Delete";
        case KeyType::ADD:
            return "Add";
        case KeyType::SUBTRACT:
            return "Substract";
        case KeyType::MULTIPLY:
            return "Multiply";
        case KeyType::DIVIDE:
            return "Divide";
        case KeyType::LEFT:
            return "Left";
        case KeyType::RIGHT:
            return "Right";
        case KeyType::UP:
            return "Up";
        case KeyType::DOWN:
            return "Down";
        case KeyType::NUMPAD0:
            return "Numpad 0";
        case KeyType::NUMPAD1:
            return "Numpad 1";
        case KeyType::NUMPAD2:
            return "Numpad 2";
        case KeyType::NUMPAD3:
            return "Numpad 3";
        case KeyType::NUMPAD4:
            return "Numpad 4";
        case KeyType::NUMPAD5:
            return "Numpad 5";
        case KeyType::NUMPAD6:
            return "Numpad 6";
        case KeyType::NUMPAD7:
            return "Numpad 7";
        case KeyType::NUMPAD8:
            return "Numpad 8";
        case KeyType::NUMPAD9:
            return "Numpad 9";
        case KeyType::F1:
            return "F1";
        case KeyType::F2:
            return "F2";
        case KeyType::F3:
            return "F3";
        case KeyType::F4:
            return "F4";
        case KeyType::F5:
            return "F5";
        case KeyType::F6:
            return "F6";
        case KeyType::F7:
            return "F7";
        case KeyType::F8:
            return "F8";
        case KeyType::F9:
            return "F9";
        case KeyType::F10:
            return "F10";
        case KeyType::F11:
            return "F11";
        case KeyType::F12:
            return "F12";
        case KeyType::F13:
            return "F13";
        case KeyType::F14:
            return "F14";
        case KeyType::F15:
            return "F15";
        case KeyType::PAUSE:
            return "Pause";
        default:
            return "Unknown Key";
    }
}

static std::string KeyState(const Event::KeyEvent &event)
{
    if (event.Shift || event.Control || event.Alt || event.System)
    {
        std::ostringstream buffer;
        buffer << " [";

        if (event.Shift)
        {
            buffer << "Shift";
        }
        if (event.Control)
        {
            buffer << "Control";
        }
        if (event.Alt)
        {
            buffer << "Alt";
        }
        if (event.System)
        {
            buffer << "System";
        }

        buffer << "]";
        return buffer.str();
    }
    return "";
}

static void PrintEvent(const Event &event)
{
    switch (event.Type)
    {
        case Event::EVENT_CLOSED:
            std::cout << "Event: Window Closed" << std::endl;
            break;
        case Event::EVENT_MOVED:
            std::cout << "Event: Window Moved (" << event.Move.X << ", " << event.Move.Y << ")"
                      << std::endl;
            break;
        case Event::EVENT_RESIZED:
            std::cout << "Event: Window Resized (" << event.Size.Width << ", " << event.Size.Height
                      << ")" << std::endl;
            break;
        case Event::EVENT_LOST_FOCUS:
            std::cout << "Event: Window Lost Focus" << std::endl;
            break;
        case Event::EVENT_GAINED_FOCUS:
            std::cout << "Event: Window Gained Focus" << std::endl;
            break;
        case Event::EVENT_TEXT_ENTERED:
            // TODO(cwallez) show the character
            std::cout << "Event: Text Entered" << std::endl;
            break;
        case Event::EVENT_KEY_PRESSED:
            std::cout << "Event: Key Pressed (" << KeyName(event.Key.Code) << KeyState(event.Key)
                      << ")" << std::endl;
            break;
        case Event::EVENT_KEY_RELEASED:
            std::cout << "Event: Key Released (" << KeyName(event.Key.Code) << KeyState(event.Key)
                      << ")" << std::endl;
            break;
        case Event::EVENT_MOUSE_WHEEL_MOVED:
            std::cout << "Event: Mouse Wheel (" << event.MouseWheel.Delta << ")" << std::endl;
            break;
        case Event::EVENT_MOUSE_BUTTON_PRESSED:
            std::cout << "Event: Mouse Button Pressed " << MouseButtonName(event.MouseButton.Button)
                      << " at (" << event.MouseButton.X << ", " << event.MouseButton.Y << ")"
                      << std::endl;
            break;
        case Event::EVENT_MOUSE_BUTTON_RELEASED:
            std::cout << "Event: Mouse Button Released "
                      << MouseButtonName(event.MouseButton.Button) << " at (" << event.MouseButton.X
                      << ", " << event.MouseButton.Y << ")" << std::endl;
            break;
        case Event::EVENT_MOUSE_MOVED:
            std::cout << "Event: Mouse Moved (" << event.MouseMove.X << ", " << event.MouseMove.Y
                      << ")" << std::endl;
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

OSWindow::OSWindow() : mX(0), mY(0), mWidth(0), mHeight(0), mValid(false), mIgnoreSizeEvents(false)
{}

OSWindow::~OSWindow() {}

bool OSWindow::initialize(const std::string &name, int width, int height)
{
    mValid = initializeImpl(name, width, height);
    return mValid;
}

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

bool OSWindow::takeScreenshot(uint8_t *pixelData)
{
    return false;
}

void *OSWindow::getPlatformExtension()
{
    return reinterpret_cast<void *>(getNativeWindow());
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
            mWidth  = event.Size.Width;
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

// static
void OSWindow::Delete(OSWindow **window)
{
    delete *window;
    *window = nullptr;
}

namespace angle
{
bool FindTestDataPath(const char *searchPath, char *dataPathOut, size_t maxDataPathOutLen)
{
#if defined(ANGLE_PLATFORM_ANDROID)
    const std::string searchPaths[] = {
        AndroidWindow::GetExternalStorageDirectory(),
        AndroidWindow::GetExternalStorageDirectory() + "/third_party/angle",
        AndroidWindow::GetApplicationDirectory() + "/chromium_tests_root"};
#elif ANGLE_PLATFORM_IOS_FAMILY
    const std::string searchPaths[] = {GetExecutableDirectory(),
                                       GetExecutableDirectory() + "/third_party/angle"};
#else
    const std::string searchPaths[] = {
        GetExecutableDirectory(), GetExecutableDirectory() + "/../..", ".",
        GetExecutableDirectory() + "/../../third_party/angle", "third_party/angle"};
#endif  // defined(ANGLE_PLATFORM_ANDROID)

    for (const std::string &path : searchPaths)
    {
        std::stringstream pathStream;
        pathStream << path << "/" << searchPath;
        std::string candidatePath = pathStream.str();

        if (candidatePath.size() + 1 >= maxDataPathOutLen)
        {
            ERR() << "FindTestDataPath: Path too long.";
            return false;
        }

        if (angle::IsDirectory(candidatePath.c_str()))
        {
            memcpy(dataPathOut, candidatePath.c_str(), candidatePath.size() + 1);
            return true;
        }

        std::ifstream inFile(candidatePath.c_str());
        if (!inFile.fail())
        {
            memcpy(dataPathOut, candidatePath.c_str(), candidatePath.size() + 1);
            return true;
        }
    }

    return false;
}
}  // namespace angle
