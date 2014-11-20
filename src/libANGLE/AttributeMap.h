//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef LIBANGLE_ATTRIBUTEMAP_H_
#define LIBANGLE_ATTRIBUTEMAP_H_

#include "libANGLE/export.h"

#include <EGL/egl.h>

#include <map>

namespace egl
{

class ANGLE_EXPORT AttributeMap
{
  public:
    AttributeMap();
    explicit AttributeMap(const EGLint *attributes);

    virtual void insert(EGLint key, EGLint value);
    virtual bool contains(EGLint key) const;
    virtual EGLint get(EGLint key, EGLint defaultValue) const;

  private:
    std::map<EGLint, EGLint> mAttributes;
};

}

#endif   // LIBANGLE_ATTRIBUTEMAP_H_
