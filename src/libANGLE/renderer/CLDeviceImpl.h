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
    struct Info
    {
        Info();
        ~Info();

        Info(const Info &) = delete;
        Info &operator=(const Info &) = delete;

        Info(Info &&);
        Info &operator=(Info &&);

        bool isValid() const { return mVersion != 0u; }

        cl_version mVersion = 0u;
        std::vector<size_t> mMaxWorkItemSizes;
        NameVersionVector mILsWithVersion;
        NameVersionVector mBuiltInKernelsWithVersion;
        NameVersionVector mOpenCL_C_AllVersions;
        NameVersionVector mOpenCL_C_Features;
        std::string mExtensions;
        NameVersionVector mExtensionsWithVersion;
        std::vector<cl_device_partition_property> mPartitionProperties;
        std::vector<cl_device_partition_property> mPartitionType;
    };

    using Ptr     = std::unique_ptr<CLDeviceImpl>;
    using PtrList = std::list<Ptr>;
    using List    = std::list<CLDeviceImpl *>;

    CLDeviceImpl(CLPlatformImpl &platform, CLDeviceImpl *parent);
    virtual ~CLDeviceImpl();

    template <typename T>
    T &getPlatform() const
    {
        return static_cast<T &>(mPlatform);
    }

    virtual Info createInfo() const = 0;

    virtual cl_int getInfoUInt(cl::DeviceInfo name, cl_uint *value) const             = 0;
    virtual cl_int getInfoULong(cl::DeviceInfo name, cl_ulong *value) const           = 0;
    virtual cl_int getInfoSizeT(cl::DeviceInfo name, size_t *value) const             = 0;
    virtual cl_int getInfoStringLength(cl::DeviceInfo name, size_t *value) const      = 0;
    virtual cl_int getInfoString(cl::DeviceInfo name, size_t size, char *value) const = 0;

    virtual cl_int createSubDevices(const cl_device_partition_property *properties,
                                    cl_uint numDevices,
                                    PtrList &implList,
                                    cl_uint *numDevicesRet) = 0;

  protected:
    CLPlatformImpl &mPlatform;
    CLDeviceImpl *const mParent;

    List mSubDevices;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CLDEVICEIMPL_H_
