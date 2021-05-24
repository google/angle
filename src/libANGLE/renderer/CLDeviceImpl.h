//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLDeviceImpl.h: Defines the abstract rx::CLDeviceImpl class.

#ifndef LIBANGLE_RENDERER_CLDEVICEIMPL_H_
#define LIBANGLE_RENDERER_CLDEVICEIMPL_H_

#include "libANGLE/renderer/CLtypes.h"

namespace rx
{

class CLDeviceImpl : angle::NonCopyable
{
  public:
    using Ptr = std::unique_ptr<CLDeviceImpl>;

    struct Info
    {
        Info();
        explicit Info(cl_device_type type);
        ~Info();

        Info(const Info &) = delete;
        Info &operator=(const Info &) = delete;

        Info(Info &&);
        Info &operator=(Info &&);

        bool isValid() const { return mType != 0u; }

        // In the order as they appear in the OpenCL specification V3.0.7, table 5
        cl_device_type mType = 0u;
        std::vector<size_t> mMaxWorkItemSizes;
        cl_ulong mMaxMemAllocSize = 0u;
        cl_bool mImageSupport     = CL_FALSE;
        std::string mIL_Version;
        NameVersionVector mILsWithVersion;
        size_t mImage2D_MaxWidth           = 0u;
        size_t mImage2D_MaxHeight          = 0u;
        size_t mImage3D_MaxWidth           = 0u;
        size_t mImage3D_MaxHeight          = 0u;
        size_t mImage3D_MaxDepth           = 0u;
        size_t mImageMaxBufferSize         = 0u;
        size_t mImageMaxArraySize          = 0u;
        cl_uint mImagePitchAlignment       = 0u;
        cl_uint mImageBaseAddressAlignment = 0u;
        std::string mBuiltInKernels;
        NameVersionVector mBuiltInKernelsWithVersion;
        std::string mVersionStr;
        cl_version mVersion = 0u;
        NameVersionVector mOpenCL_C_AllVersions;
        NameVersionVector mOpenCL_C_Features;
        std::string mExtensions;
        NameVersionVector mExtensionsWithVersion;
        std::vector<cl_device_partition_property> mPartitionProperties;
        std::vector<cl_device_partition_property> mPartitionType;
    };

    CLDeviceImpl(const cl::Device &device);
    virtual ~CLDeviceImpl();

    virtual Info createInfo(cl_device_type type) const = 0;

    virtual cl_int getInfoUInt(cl::DeviceInfo name, cl_uint *value) const             = 0;
    virtual cl_int getInfoULong(cl::DeviceInfo name, cl_ulong *value) const           = 0;
    virtual cl_int getInfoSizeT(cl::DeviceInfo name, size_t *value) const             = 0;
    virtual cl_int getInfoStringLength(cl::DeviceInfo name, size_t *value) const      = 0;
    virtual cl_int getInfoString(cl::DeviceInfo name, size_t size, char *value) const = 0;

    virtual cl_int createSubDevices(cl::Device &device,
                                    const cl_device_partition_property *properties,
                                    cl_uint numDevices,
                                    cl::DevicePtrList &subDeviceList,
                                    cl_uint *numDevicesRet) = 0;

  protected:
    const cl::Device &mDevice;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CLDEVICEIMPL_H_
