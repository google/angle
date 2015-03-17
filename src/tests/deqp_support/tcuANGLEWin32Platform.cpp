/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "tcuANGLEWin32Platform.h"
#include "tcuANGLEWin32NativeDisplayFactory.h"
#include "egluGLContextFactory.hpp"

namespace tcu
{

ANGLEWin32Platform::ANGLEWin32Platform()
    : mInstance(GetModuleHandle(nullptr))
{
    // Set process priority to lower.
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);

    m_nativeDisplayFactoryRegistry.registerFactory(new ANGLEWin32NativeDisplayFactory(mInstance));
    m_contextFactoryRegistry.registerFactory(new eglu::GLContextFactory(m_nativeDisplayFactoryRegistry));
}

ANGLEWin32Platform::~ANGLEWin32Platform()
{
}

bool ANGLEWin32Platform::processEvents()
{
    MSG msg;
    while (PeekMessage(&msg, reinterpret_cast<HWND>(-1), 0, 0, PM_REMOVE))
    {
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
            return false;
    }
    return true;
}

} // tcu

// Create platform
tcu::Platform *createPlatform()
{
    return new tcu::ANGLEWin32Platform();
}
