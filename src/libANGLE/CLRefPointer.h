//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLRefPointer.h: A non-owning intrinsic reference counting smart pointer for CL objects.

#ifndef LIBANGLE_CLREFPOINTER_H_
#define LIBANGLE_CLREFPOINTER_H_

#include <algorithm>

namespace cl
{

template <typename T>
class RefPointer
{
  public:
    RefPointer() noexcept : mCLObject(nullptr) {}

    explicit RefPointer(T *object) noexcept : mCLObject(object)
    {
        if (mCLObject != nullptr)
        {
            mCLObject->retain();
        }
    }
    ~RefPointer()
    {
        if (mCLObject != nullptr)
        {
            mCLObject->release();
        }
    }

    RefPointer(std::nullptr_t) noexcept : mCLObject(nullptr) {}
    RefPointer &operator=(std::nullptr_t)
    {
        reset();
        return *this;
    }

    RefPointer(RefPointer &&other) noexcept : mCLObject(nullptr) { other.swap(*this); }
    RefPointer &operator=(RefPointer &&other)
    {
        other.swap(this);
        return *this;
    }

    RefPointer(const RefPointer<T> &other) : mCLObject(other.mCLObject)
    {
        if (mCLObject != nullptr)
        {
            mCLObject->retain();
        }
    }
    RefPointer &operator=(const RefPointer<T> &other)
    {
        if (this != &other)
        {
            reset();
            mCLObject = other.mCLObject;
            if (mCLObject != nullptr)
            {
                mCLObject->retain();
            }
        }
        return *this;
    }

    T *operator->() const { return mCLObject; }
    T &operator*() const { return *mCLObject; }

    T *get() const { return mCLObject; }
    explicit operator bool() const { return mCLObject != nullptr; }

    T *release() noexcept
    {
        T *const object = mCLObject;
        mCLObject       = nullptr;
        return object;
    }

    void swap(RefPointer &other) noexcept { std::swap(mCLObject, other.mCLObject); }

    void reset()
    {
        if (mCLObject != nullptr)
        {
            T *const object = release();
            object->release();
        }
    }

  private:
    T *mCLObject;
};

template <typename T>
void swap(RefPointer<T> &left, RefPointer<T> &right)
{
    left.swap(right);
}

}  // namespace cl

#endif  // LIBANGLE_CLREFPOINTER_H_
