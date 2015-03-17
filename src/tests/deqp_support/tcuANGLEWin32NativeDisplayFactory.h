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

#ifndef TCU_ANGLE_WIN32_NATIVE_DISPLAY_FACTORY_H_
#define TCU_ANGLE_WIN32_NATIVE_DISPLAY_FACTORY_H_

#include "tcuDefs.hpp"
#include "egluNativeDisplay.hpp"
#include "eglwDefs.hpp"
#include "tcuWin32API.h"

namespace tcu
{

class ANGLEWin32NativeDisplayFactory : public eglu::NativeDisplayFactory
{
  public:
    ANGLEWin32NativeDisplayFactory(HINSTANCE instance);
    ~ANGLEWin32NativeDisplayFactory() override;

    eglu::NativeDisplay *createDisplay(const eglw::EGLAttrib* attribList) const override;

  private:
    const HINSTANCE mInstance;
};

} // tcu

#endif // TCU_ANGLE_WIN32_NATIVE_DISPLAY_FACTORY_H_
