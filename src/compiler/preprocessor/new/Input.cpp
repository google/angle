//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "Input.h"

#include <algorithm>
#include <cassert>

namespace pp
{

Input::Input() : mCount(0), mString(0), mLength(0)
{
}

Input::Input(int count, const char* const string[], const int length[]) :
    mCount(count),
    mString(string),
    mLength(0)
{
    assert(mCount >= 0);
    mLength = mCount > 0 ? new int[mCount] : 0;
    for (int i = 0; i < mCount; ++i)
    {
        mLength[i] = length ? length[i] : -1;
        if (mLength[i] < 0)
            mLength[i] = strlen(mString[i]);
    }
}

Input::~Input()
{
    if (mLength) delete [] mLength;
}

int Input::read(char* buf, int maxSize)
{
    int nRead = 0;
    while ((nRead < maxSize) && (mReadLoc.sIndex < mCount))
    {
        int size = mLength[mReadLoc.sIndex] - mReadLoc.cIndex;
        size = std::min(size, maxSize);
        memcpy(buf + nRead, mString[mReadLoc.sIndex] + mReadLoc.cIndex, size);
        nRead += size;
        mReadLoc.cIndex += size;

        // Advance string if we reached the end of current string.
        if (mReadLoc.cIndex == mLength[mReadLoc.sIndex])
        {
            ++mReadLoc.sIndex;
            mReadLoc.cIndex = 0;
        }
    }
    return nRead;
}

}  // namespace pp

