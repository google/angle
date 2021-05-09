//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLDeviceCL.cpp: Implements the class methods for CLDeviceCL.

#include "libANGLE/renderer/cl/CLDeviceCL.h"

#include "libANGLE/renderer/cl/CLPlatformCL.h"
#include "libANGLE/renderer/cl/cl_util.h"

#include "libANGLE/Debug.h"

namespace rx
{

namespace
{

// Object information is queried in OpenCL by providing allocated memory into which the requested
// data is copied. If the size of the data is unknown, it can be queried first with an additional
// call to the same function, but without requesting the data itself. This function provides the
// functionality to request and validate the size and the data.
template <typename T>
bool GetDeviceInfo(cl_device_id device, cl::DeviceInfo name, std::vector<T> &vector)
{
    size_t size = 0u;
    if (device->getDispatch().clGetDeviceInfo(device, cl::ToCLenum(name), 0u, nullptr, &size) ==
            CL_SUCCESS &&
        (size % sizeof(T)) == 0u)  // size has to be a multiple of the data type
    {
        vector.resize(size / sizeof(T));
        if (device->getDispatch().clGetDeviceInfo(device, cl::ToCLenum(name), size, vector.data(),
                                                  nullptr) == CL_SUCCESS)
        {
            return true;
        }
    }
    ERR() << "Failed to query CL device info for " << name;
    return false;
}

}  // namespace

CLDeviceCL::~CLDeviceCL()
{
    if (mVersion >= CL_MAKE_VERSION(1, 2, 0) &&
        mDevice->getDispatch().clReleaseDevice(mDevice) != CL_SUCCESS)
    {
        ERR() << "Error while releasing CL device";
    }
}

CLDeviceImpl::Info CLDeviceCL::createInfo() const
{
    Info info;
    info.mVersion = mVersion;

    std::vector<char> valString;
    if (!GetDeviceInfo(mDevice, cl::DeviceInfo::Extensions, valString))
    {
        return Info{};
    }
    info.mExtensions.assign(valString.data());
    RemoveUnsupportedCLExtensions(info.mExtensions);

    if (!GetDeviceInfo(mDevice, cl::DeviceInfo::MaxWorkItemSizes, info.mMaxWorkItemSizes))
    {
        return Info{};
    }
    // From the OpenCL specification for info name CL_DEVICE_MAX_WORK_ITEM_SIZES:
    // "The minimum value is (1, 1, 1) for devices that are not of type CL_DEVICE_TYPE_CUSTOM."
    // https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clGetDeviceInfo
    // Custom devices are currently not supported by this back end.
    if (info.mMaxWorkItemSizes.size() < 3u || info.mMaxWorkItemSizes[0] == 0u ||
        info.mMaxWorkItemSizes[1] == 0u || info.mMaxWorkItemSizes[2] == 0u)
    {
        ERR() << "Invalid CL_DEVICE_MAX_WORK_ITEM_SIZES";
        return Info{};
    }

    if (mVersion >= CL_MAKE_VERSION(1, 2, 0) &&
        (!GetDeviceInfo(mDevice, cl::DeviceInfo::PartitionProperties, info.mPartitionProperties) ||
         !GetDeviceInfo(mDevice, cl::DeviceInfo::PartitionType, info.mPartitionType)))
    {
        return Info{};
    }

    if (mVersion >= CL_MAKE_VERSION(3, 0, 0) &&
        (!GetDeviceInfo(mDevice, cl::DeviceInfo::ILsWithVersion, info.mILsWithVersion) ||
         !GetDeviceInfo(mDevice, cl::DeviceInfo::BuiltInKernelsWithVersion,
                        info.mBuiltInKernelsWithVersion) ||
         !GetDeviceInfo(mDevice, cl::DeviceInfo::OpenCL_C_AllVersions,
                        info.mOpenCL_C_AllVersions) ||
         !GetDeviceInfo(mDevice, cl::DeviceInfo::OpenCL_C_Features, info.mOpenCL_C_Features) ||
         !GetDeviceInfo(mDevice, cl::DeviceInfo::ExtensionsWithVersion,
                        info.mExtensionsWithVersion)))
    {
        return Info{};
    }
    RemoveUnsupportedCLExtensions(info.mExtensionsWithVersion);

    return info;
}

cl_int CLDeviceCL::getInfoUInt(cl::DeviceInfo name, cl_uint *value) const
{
    return mDevice->getDispatch().clGetDeviceInfo(mDevice, cl::ToCLenum(name), sizeof(*value),
                                                  value, nullptr);
}

cl_int CLDeviceCL::getInfoULong(cl::DeviceInfo name, cl_ulong *value) const
{
    return mDevice->getDispatch().clGetDeviceInfo(mDevice, cl::ToCLenum(name), sizeof(*value),
                                                  value, nullptr);
}

cl_int CLDeviceCL::getInfoSizeT(cl::DeviceInfo name, size_t *value) const
{
    return mDevice->getDispatch().clGetDeviceInfo(mDevice, cl::ToCLenum(name), sizeof(*value),
                                                  value, nullptr);
}

cl_int CLDeviceCL::getInfoStringLength(cl::DeviceInfo name, size_t *value) const
{
    return mDevice->getDispatch().clGetDeviceInfo(mDevice, cl::ToCLenum(name), 0u, nullptr, value);
}

cl_int CLDeviceCL::getInfoString(cl::DeviceInfo name, size_t size, char *value) const
{
    return mDevice->getDispatch().clGetDeviceInfo(mDevice, cl::ToCLenum(name), size, value,
                                                  nullptr);
}

cl_int CLDeviceCL::createSubDevices(const cl_device_partition_property *properties,
                                    cl_uint numDevices,
                                    PtrList &implList,
                                    cl_uint *numDevicesRet)
{
    if (mVersion < CL_MAKE_VERSION(1, 2, 0))
    {
        return CL_INVALID_VALUE;
    }
    if (numDevices == 0u)
    {
        return mDevice->getDispatch().clCreateSubDevices(mDevice, properties, 0u, nullptr,
                                                         numDevicesRet);
    }

    std::vector<cl_device_id> devices(numDevices, nullptr);
    const cl_int result = mDevice->getDispatch().clCreateSubDevices(mDevice, properties, numDevices,
                                                                    devices.data(), nullptr);
    if (result == CL_SUCCESS)
    {
        for (cl_device_id device : devices)
        {
            implList.emplace_back(CLDeviceCL::Create(getPlatform<CLPlatformCL>(), this, device));
            if (!implList.back())
            {
                implList.clear();
                return CL_INVALID_VALUE;
            }
            mSubDevices.emplace_back(implList.back().get());
        }
    }
    return result;
}

CLDeviceCL *CLDeviceCL::Create(CLPlatformCL &platform, CLDeviceCL *parent, cl_device_id device)
{
    size_t valueSize = 0u;
    if (device->getDispatch().clGetDeviceInfo(device, CL_DEVICE_VERSION, 0u, nullptr, &valueSize) ==
        CL_SUCCESS)
    {
        std::vector<char> valString(valueSize, '\0');
        if (device->getDispatch().clGetDeviceInfo(device, CL_DEVICE_VERSION, valueSize,
                                                  valString.data(), nullptr) == CL_SUCCESS)
        {
            const cl_version version = ExtractCLVersion(valString.data());
            if (version != 0u)
            {
                return new CLDeviceCL(platform, parent, device, version);
            }
        }
    }
    ERR() << "Failed to query version for device";
    return nullptr;
}

CLDeviceCL::CLDeviceCL(CLPlatformCL &platform,
                       CLDeviceCL *parent,
                       cl_device_id device,
                       cl_version version)
    : CLDeviceImpl(platform, parent), mDevice(device), mVersion(version)
{}

}  // namespace rx
