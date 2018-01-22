//
// Copyright (c) 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ImmutableString.h: Wrapper for static or pool allocated char arrays, that are guaranteed to be
// valid and unchanged for the duration of the compilation.
//

#ifndef COMPILER_TRANSLATOR_IMMUTABLESTRING_H_
#define COMPILER_TRANSLATOR_IMMUTABLESTRING_H_

#include <string>

#include "compiler/translator/Common.h"

namespace sh
{

namespace
{
constexpr size_t constStrlen(const char *str)
{
    if (str == nullptr)
    {
        return 0u;
    }
    size_t len = 0u;
    while (*(str + len) != '\0')
    {
        ++len;
    }
    return len;
}
}

class ImmutableString
{
  public:
    // The data pointer passed in must be one of:
    //  1. nullptr (only valid with length 0).
    //  2. a null-terminated static char array like a string literal.
    //  3. a null-terminated pool allocated char array.
    explicit constexpr ImmutableString(const char *data) : mData(data), mLength(constStrlen(data))
    {
    }

    ImmutableString(const ImmutableString &) = default;
    ImmutableString &operator=(const ImmutableString &) = default;

    const char *data() const { return mData ? mData : ""; }

    bool operator<(const ImmutableString &b) const
    {
        if (mLength < b.mLength)
        {
            return true;
        }
        if (mLength > b.mLength)
        {
            return false;
        }
        return (memcmp(data(), b.data(), mLength) < 0);
    }

  private:
    const char *const mData;
    const size_t mLength;
};

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_IMMUTABLESTRING_H_
