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

#ifndef TCU_ANGLE_WIN32_PLATFORM_H_
#define TCU_ANGLE_WIN32_PLATFORM_H_

#include "tcuDefs.hpp"
#include "tcuPlatform.hpp"
#include "gluPlatform.hpp"
#include "tcuWin32API.h"

#ifndef _EGLUPLATFORM_HPP
#   include "egluPlatform.hpp"
#endif

namespace tcu
{

class ANGLEWin32Platform : public tcu::Platform,
                           private glu::Platform,
                           private eglu::Platform
{
  public:
    ANGLEWin32Platform();
    ~ANGLEWin32Platform();

    bool processEvents() override;

    const glu::Platform &getGLPlatform() const override { return static_cast<const glu::Platform&>(*this); }
    const eglu::Platform &getEGLPlatform() const override { return static_cast<const eglu::Platform&>(*this); }

  private:
    HINSTANCE mInstance;
};

} // tcu

#endif // TCU_ANGLE_WIN32_PLATFORM_H_
