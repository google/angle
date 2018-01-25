//
// Copyright (c) 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ImmutableStringBuilder.h: Stringstream-like utility for building pool allocated strings where the
// maximum length is known in advance.
//

#ifndef COMPILER_TRANSLATOR_IMMUTABLESTRINGBUILDER_H_
#define COMPILER_TRANSLATOR_IMMUTABLESTRINGBUILDER_H_

#include "compiler/translator/ImmutableString.h"

namespace sh
{

class ImmutableStringBuilder
{
  public:
    ImmutableStringBuilder(size_t maxLength)
        : mPos(0u), mMaxLength(maxLength), mData(AllocateEmptyPoolCharArray(maxLength))
    {
    }

    ImmutableStringBuilder &operator<<(const ImmutableString &str);

    ImmutableStringBuilder &operator<<(const char *str);

    ImmutableStringBuilder &operator<<(const char &c);

    // This invalidates the ImmutableStringBuilder, so it should only be called once.
    operator ImmutableString();

  private:
    inline static char *AllocateEmptyPoolCharArray(size_t strLength)
    {
        size_t requiredSize = strLength + 1u;
        return reinterpret_cast<char *>(GetGlobalPoolAllocator()->allocate(requiredSize));
    }

    size_t mPos;
    size_t mMaxLength;
    char *mData;
};

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_IMMUTABLESTRINGBUILDER_H_
