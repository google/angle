//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RefCountObject.cpp: Defines the gl::RefCountObject base class that provides
// lifecycle support for GL objects using the traditional BindObject scheme, but
// that need to be reference counted for correct cross-context deletion.
// (Concretely, textures, buffers and renderbuffers.)

#include "main.h"

#include "RefCountObject.h"

namespace gl
{

RefCountObject::RefCountObject(GLuint id)
{
    mId = id;
    mRefCount = 0;
    mIsDeleted = false;
}

RefCountObject::~RefCountObject()
{
}

void RefCountObject::addRef() const
{
    mRefCount++;
}

void RefCountObject::release() const
{
    ASSERT(mRefCount > 0);

    if (--mRefCount == 0)
    {
        delete this;
    }
}

GLuint RefCountObject::id() const
{
    if (mIsDeleted)
    {
        return error(GL_INVALID_OPERATION, 0);
    }
    
    return mId;
}

void RefCountObject::markAsDeleted()
{
    mId = 0;
    mIsDeleted = true;
}

void RefCountObjectBindingPointer::set(RefCountObject *newObject)
{
    // addRef first in case newObject == mObject and this is the last reference to it.
    if (newObject != NULL) newObject->addRef();
    if (mObject != NULL) mObject->release();

    mObject = newObject;
}

}
