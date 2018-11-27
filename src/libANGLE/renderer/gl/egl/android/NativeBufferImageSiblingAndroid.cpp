//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// NativeBufferImageSiblingAndroid.cpp: Implements the NativeBufferImageSiblingAndroid class

#include "libANGLE/renderer/gl/egl/android/NativeBufferImageSiblingAndroid.h"

#include "libANGLE/renderer/gl/egl/android/android_util.h"

// Taken from cutils/native_handle.h:
// https://android.googlesource.com/platform/system/core/+/master/libcutils/include/cutils/native_handle.h
typedef struct native_handle
{
    int version; /* sizeof(native_handle_t) */
    int numFds;  /* number of file-descriptors at &data[0] */
    int numInts; /* number of ints at &data[numFds] */
    int data[0]; /* numFds + numInts ints */
} native_handle_t;

// Taken from nativebase/nativebase.h
// https://android.googlesource.com/platform/frameworks/native/+/master/libs/nativebase/include/nativebase/nativebase.h
typedef const native_handle_t *buffer_handle_t;

typedef struct android_native_base_t
{
    /* a magic value defined by the actual EGL native type */
    int magic;
    /* the sizeof() of the actual EGL native type */
    int version;
    void *reserved[4];
    /* reference-counting interface */
    void (*incRef)(struct android_native_base_t *base);
    void (*decRef)(struct android_native_base_t *base);
} android_native_base_t;

typedef struct ANativeWindowBuffer
{
    struct android_native_base_t common;
    int width;
    int height;
    int stride;
    int format;
    int usage_deprecated;
    uintptr_t layerCount;
    void *reserved[1];
    const native_handle_t *handle;
    uint64_t usage;
    // we needed extra space for storing the 64-bits usage flags
    // the number of slots to use from reserved_proc depends on the
    // architecture.
    void *reserved_proc[8 - (sizeof(uint64_t) / sizeof(void *))];
} ANativeWindowBuffer_t;

namespace rx
{
NativeBufferImageSiblingAndroid::NativeBufferImageSiblingAndroid(EGLClientBuffer buffer)
    : mBuffer(static_cast<struct ANativeWindowBuffer *>(buffer))
{}

NativeBufferImageSiblingAndroid::~NativeBufferImageSiblingAndroid() {}

gl::Format NativeBufferImageSiblingAndroid::getFormat() const
{
    return gl::Format(android::NativePixelFormatToGLInternalFormat(mBuffer->format));
}

bool NativeBufferImageSiblingAndroid::isRenderable(const gl::Context *context) const
{
    return true;
}

bool NativeBufferImageSiblingAndroid::isTexturable(const gl::Context *context) const
{
    return true;
}

gl::Extents NativeBufferImageSiblingAndroid::getSize() const
{
    return gl::Extents(mBuffer->width, mBuffer->height, 1);
}

size_t NativeBufferImageSiblingAndroid::getSamples() const
{
    return 0;
}

EGLClientBuffer NativeBufferImageSiblingAndroid::getBuffer() const
{
    return static_cast<EGLClientBuffer>(mBuffer);
}

}  // namespace rx
