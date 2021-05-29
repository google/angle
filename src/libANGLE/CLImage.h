//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLImage.h: Defines the cl::Image class, which stores a texture, frame-buffer or image.

#ifndef LIBANGLE_CLIMAGE_H_
#define LIBANGLE_CLIMAGE_H_

#include "libANGLE/CLMemory.h"

namespace cl
{

class Image final : public Memory
{
  public:
    ~Image() override;

    cl_mem_object_type getType() const final;

    const cl_image_format &getFormat() const;
    const ImageDescriptor &getDescriptor() const;

    cl_int getInfo(ImageInfo name, size_t valueSize, void *value, size_t *valueSizeRet) const;

    static bool IsValid(const _cl_mem *image);

  private:
    Image(Context &context,
          PropArray &&properties,
          MemFlags flags,
          const cl_image_format &format,
          const ImageDescriptor &desc,
          Memory *parent,
          void *hostPtr,
          cl_int &errorCode);

    const cl_image_format mFormat;
    const ImageDescriptor mDesc;

    friend class Object;
};

inline cl_mem_object_type Image::getType() const
{
    return mDesc.type;
}

inline const cl_image_format &Image::getFormat() const
{
    return mFormat;
}

inline const ImageDescriptor &Image::getDescriptor() const
{
    return mDesc;
}

}  // namespace cl

#endif  // LIBANGLE_CLIMAGE_H_
