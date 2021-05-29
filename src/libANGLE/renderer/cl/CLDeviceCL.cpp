//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLDeviceCL.cpp: Implements the class methods for CLDeviceCL.

#include "libANGLE/renderer/cl/CLDeviceCL.h"

#include "libANGLE/renderer/cl/cl_util.h"

#include "libANGLE/CLDevice.h"

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

// This queries the OpenCL device info for value types with known size
template <typename T>
bool GetDeviceInfo(cl_device_id device, cl::DeviceInfo name, T &value)
{
    if (device->getDispatch().clGetDeviceInfo(device, cl::ToCLenum(name), sizeof(T), &value,
                                              nullptr) != CL_SUCCESS)
    {
        ERR() << "Failed to query CL device info for " << name;
        return false;
    }
    return true;
}

}  // namespace

CLDeviceCL::~CLDeviceCL()
{
    if (!mDevice.isRoot() && mNative->getDispatch().clReleaseDevice(mNative) != CL_SUCCESS)
    {
        ERR() << "Error while releasing CL device";
    }
}

CLDeviceImpl::Info CLDeviceCL::createInfo(cl::DeviceType type) const
{
    Info info(type);
    std::vector<char> valString;

    if (!GetDeviceInfo(mNative, cl::DeviceInfo::MaxWorkItemSizes, info.mMaxWorkItemSizes))
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

    if (!GetDeviceInfo(mNative, cl::DeviceInfo::MaxMemAllocSize, info.mMaxMemAllocSize) ||
        !GetDeviceInfo(mNative, cl::DeviceInfo::ImageSupport, info.mImageSupport) ||
        !GetDeviceInfo(mNative, cl::DeviceInfo::Image2D_MaxWidth, info.mImage2D_MaxWidth) ||
        !GetDeviceInfo(mNative, cl::DeviceInfo::Image2D_MaxHeight, info.mImage2D_MaxHeight) ||
        !GetDeviceInfo(mNative, cl::DeviceInfo::Image3D_MaxWidth, info.mImage3D_MaxWidth) ||
        !GetDeviceInfo(mNative, cl::DeviceInfo::Image3D_MaxHeight, info.mImage3D_MaxHeight) ||
        !GetDeviceInfo(mNative, cl::DeviceInfo::Image3D_MaxDepth, info.mImage3D_MaxDepth))
    {
        return Info{};
    }

    if (!GetDeviceInfo(mNative, cl::DeviceInfo::Version, valString))
    {
        return Info{};
    }
    info.mVersionStr.assign(valString.data());
    info.mVersion = ExtractCLVersion(info.mVersionStr);
    if (info.mVersion == 0u)
    {
        return Info{};
    }

    if (!GetDeviceInfo(mNative, cl::DeviceInfo::Extensions, valString))
    {
        return Info{};
    }
    info.mExtensions.assign(valString.data());
    RemoveUnsupportedCLExtensions(info.mExtensions);

    if (info.mVersion >= CL_MAKE_VERSION(1, 2, 0))
    {
        if (!GetDeviceInfo(mNative, cl::DeviceInfo::ImageMaxBufferSize, info.mImageMaxBufferSize) ||
            !GetDeviceInfo(mNative, cl::DeviceInfo::ImageMaxArraySize, info.mImageMaxArraySize) ||
            !GetDeviceInfo(mNative, cl::DeviceInfo::BuiltInKernels, valString))
        {
            return Info{};
        }
        info.mBuiltInKernels.assign(valString.data());
        if (!GetDeviceInfo(mNative, cl::DeviceInfo::PartitionProperties,
                           info.mPartitionProperties) ||
            !GetDeviceInfo(mNative, cl::DeviceInfo::PartitionType, info.mPartitionType))
        {
            return Info{};
        }
    }

    if (info.mVersion >= CL_MAKE_VERSION(2, 0, 0) &&
        (!GetDeviceInfo(mNative, cl::DeviceInfo::ImagePitchAlignment, info.mImagePitchAlignment) ||
         !GetDeviceInfo(mNative, cl::DeviceInfo::ImageBaseAddressAlignment,
                        info.mImageBaseAddressAlignment) ||
         !GetDeviceInfo(mNative, cl::DeviceInfo::QueueOnDeviceMaxSize, info.mQueueOnDeviceMaxSize)))
    {
        return Info{};
    }

    if (info.mVersion >= CL_MAKE_VERSION(2, 1, 0))
    {
        if (!GetDeviceInfo(mNative, cl::DeviceInfo::IL_Version, valString))
        {
            return Info{};
        }
        info.mIL_Version.assign(valString.data());
    }

    if (info.mVersion >= CL_MAKE_VERSION(3, 0, 0) &&
        (!GetDeviceInfo(mNative, cl::DeviceInfo::ILsWithVersion, info.mILsWithVersion) ||
         !GetDeviceInfo(mNative, cl::DeviceInfo::BuiltInKernelsWithVersion,
                        info.mBuiltInKernelsWithVersion) ||
         !GetDeviceInfo(mNative, cl::DeviceInfo::OpenCL_C_AllVersions,
                        info.mOpenCL_C_AllVersions) ||
         !GetDeviceInfo(mNative, cl::DeviceInfo::OpenCL_C_Features, info.mOpenCL_C_Features) ||
         !GetDeviceInfo(mNative, cl::DeviceInfo::ExtensionsWithVersion,
                        info.mExtensionsWithVersion)))
    {
        return Info{};
    }
    RemoveUnsupportedCLExtensions(info.mExtensionsWithVersion);

    return info;
}

cl_int CLDeviceCL::getInfoUInt(cl::DeviceInfo name, cl_uint *value) const
{
    return mNative->getDispatch().clGetDeviceInfo(mNative, cl::ToCLenum(name), sizeof(*value),
                                                  value, nullptr);
}

cl_int CLDeviceCL::getInfoULong(cl::DeviceInfo name, cl_ulong *value) const
{
    return mNative->getDispatch().clGetDeviceInfo(mNative, cl::ToCLenum(name), sizeof(*value),
                                                  value, nullptr);
}

cl_int CLDeviceCL::getInfoSizeT(cl::DeviceInfo name, size_t *value) const
{
    return mNative->getDispatch().clGetDeviceInfo(mNative, cl::ToCLenum(name), sizeof(*value),
                                                  value, nullptr);
}

cl_int CLDeviceCL::getInfoStringLength(cl::DeviceInfo name, size_t *value) const
{
    return mNative->getDispatch().clGetDeviceInfo(mNative, cl::ToCLenum(name), 0u, nullptr, value);
}

cl_int CLDeviceCL::getInfoString(cl::DeviceInfo name, size_t size, char *value) const
{
    return mNative->getDispatch().clGetDeviceInfo(mNative, cl::ToCLenum(name), size, value,
                                                  nullptr);
}

cl_int CLDeviceCL::createSubDevices(const cl_device_partition_property *properties,
                                    cl_uint numDevices,
                                    CreateFuncs &createFuncs,
                                    cl_uint *numDevicesRet)
{
    if (numDevices == 0u)
    {
        return mNative->getDispatch().clCreateSubDevices(mNative, properties, 0u, nullptr,
                                                         numDevicesRet);
    }

    std::vector<cl_device_id> nativeSubDevices(numDevices, nullptr);
    const cl_int errorCode = mNative->getDispatch().clCreateSubDevices(
        mNative, properties, numDevices, nativeSubDevices.data(), nullptr);
    if (errorCode == CL_SUCCESS)
    {
        for (cl_device_id nativeSubDevice : nativeSubDevices)
        {
            createFuncs.emplace_back([=](const cl::Device &device) {
                return Ptr(new CLDeviceCL(device, nativeSubDevice));
            });
        }
    }
    return errorCode;
}

CLDeviceCL::CLDeviceCL(const cl::Device &device, cl_device_id native)
    : CLDeviceImpl(device), mNative(native)
{}

}  // namespace rx
