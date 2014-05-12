//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BinaryStream.h: Provides binary serialization of simple types.

#ifndef LIBGLESV2_BINARYSTREAM_H_
#define LIBGLESV2_BINARYSTREAM_H_

#include "common/angleutils.h"

namespace gl
{

class BinaryInputStream
{
  public:
    BinaryInputStream(const void *data, size_t length)
    {
        mError = false;
        mOffset = 0;
        mData = static_cast<const char*>(data);
        mLength = length;
    }

    // readInt will generate an error for bool types
    template <class IntT>
    IntT readInt()
    {
        int value;
        read(&value);
        return static_cast<IntT>(value);
    }

    template <class IntT>
    void readInt(IntT *outValue)
    {
        int value;
        read(&value);
        *outValue = static_cast<IntT>(value);
    }

    bool readBool()
    {
        int value;
        read(&value);
        return (value > 0);
    }

    void readBool(bool *outValue)
    {
        int value;
        read(&value);
        *outValue = (value > 0);
    }

    void readBytes(unsigned char outArray[], size_t count)
    {
        read<unsigned char>(outArray, count);
    }

    std::string readString()
    {
        std::string outString;
        readString(&outString);
        return outString;
    }

    void readString(std::string *v)
    {
        size_t length;
        read(&length);

        if (mError)
        {
            return;
        }

        if (mOffset + length > mLength)
        {
            mError = true;
            return;
        }

        v->assign(mData + mOffset, length);
        mOffset += length;
    }

    void skip(size_t length)
    {
        if (mOffset + length > mLength)
        {
            mError = true;
            return;
        }

        mOffset += length;
    }

    size_t offset() const
    {
        return mOffset;
    }

    bool error() const
    {
        return mError;
    }

    bool endOfStream() const
    {
        return mOffset == mLength;
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(BinaryInputStream);
    bool mError;
    size_t mOffset;
    const char *mData;
    size_t mLength;

    template <typename T>
    void read(T *v, size_t num)
    {
        union
        {
            T dummy;  // Compilation error for non-POD types
        } dummy;
        (void)dummy;

        if (mError)
        {
            return;
        }

        size_t length = num * sizeof(T);

        if (mOffset + length > mLength)
        {
            mError = true;
            return;
        }

        memcpy(v, mData + mOffset, length);
        mOffset += length;
    }

    template <typename T>
    void read(T * v)
    {
        read(v, 1);
    }

};

class BinaryOutputStream
{
  public:
    BinaryOutputStream()
    {
    }

    // writeInt also handles bool types
    template <class IntT>
    void writeInt(IntT v)
    {
        int intValue = static_cast<int>(v);
        write(&v, 1);
    }

    void writeString(const std::string &v)
    {
        writeInt(v.length());
        write(v.c_str(), v.length());
    }

    void writeBytes(unsigned char *bytes, size_t count)
    {
        write(bytes, count);
    }

    size_t length() const
    {
        return mData.size();
    }

    const void* data() const
    {
        return mData.size() ? &mData[0] : NULL;
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(BinaryOutputStream);
    std::vector<char> mData;

    template <typename T>
    void write(const T *v, size_t num)
    {
        union
        {
            T dummy;  // Compilation error for non-POD types
        } dummy;
        (void) dummy;

        const char *asBytes = reinterpret_cast<const char*>(v);
        mData.insert(mData.end(), asBytes, asBytes + num * sizeof(T));
    }

};
}

#endif  // LIBGLESV2_BINARYSTREAM_H_
