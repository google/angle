//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// mtl_common.h:
//      Declares common constants, template classes, and mtl::Context - the MTLDevice container &
//      error handler base class.
//

#ifndef LIBANGLE_RENDERER_METAL_MTL_COMMON_H_
#define LIBANGLE_RENDERER_METAL_MTL_COMMON_H_

#import <Metal/Metal.h>

#include <string>

#include "common/Optional.h"
#include "common/PackedEnums.h"
#include "common/angleutils.h"
#include "libANGLE/Constants.h"
#include "libANGLE/Version.h"
#include "libANGLE/angletypes.h"

namespace rx
{
class RendererMtl;

namespace mtl
{

class ErrorHandler
{
  public:
    virtual ~ErrorHandler() {}

    virtual void handleError(GLenum error,
                             const char *file,
                             const char *function,
                             unsigned int line) = 0;
};

class Context : public ErrorHandler
{
  public:
    Context(RendererMtl *rendererMtl);
    _Nullable id<MTLDevice> getMetalDevice() const;

    RendererMtl *getRenderer() const { return mRendererMtl; }

  protected:
    RendererMtl *mRendererMtl;
};

#define ANGLE_MTL_CHECK(context, test, error)                                \
    do                                                                       \
    {                                                                        \
        if (ANGLE_UNLIKELY(!test))                                           \
        {                                                                    \
            context->handleError(error, __FILE__, ANGLE_FUNCTION, __LINE__); \
            return angle::Result::Stop;                                      \
        }                                                                    \
    } while (0)

#define ANGLE_MTL_TRY(context, test) ANGLE_MTL_CHECK(context, test, GL_INVALID_OPERATION)

}  // namespace mtl
}  // namespace rx

#endif /* LIBANGLE_RENDERER_METAL_MTL_COMMON_H_ */
