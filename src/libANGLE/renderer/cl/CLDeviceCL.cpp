//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLDeviceCL.cpp: Implements the class methods for CLDeviceCL.

#include "libANGLE/renderer/cl/CLDeviceCL.h"

#include "libANGLE/renderer/cl/cl_util.h"

#include "libANGLE/Debug.h"

namespace rx
{

CLDeviceCL::~CLDeviceCL()
{
    if (mVersion >= CL_MAKE_VERSION(1, 2, 0) &&
        mDevice->getDispatch().clReleaseDevice(mDevice) != CL_SUCCESS)
    {
        ERR() << "Error while releasing CL device";
    }
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
                                    InitList &deviceInitList,
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
            CLDeviceImpl::Ptr impl(CLDeviceCL::Create(device));
            CLDeviceImpl::Info info = CLDeviceCL::GetInfo(device);
            if (impl && info.isValid())
            {
                deviceInitList.emplace_back(std::move(impl), std::move(info));
            }
        }
        if (deviceInitList.size() != devices.size())
        {
            return CL_INVALID_VALUE;
        }
    }
    return result;
}

#define ANGLE_GET_INFO_SIZE(name, size_ret) \
    device->getDispatch().clGetDeviceInfo(device, name, 0u, nullptr, size_ret)

#define ANGLE_GET_INFO_SIZE_RET(name, size_ret)                     \
    do                                                              \
    {                                                               \
        if (ANGLE_GET_INFO_SIZE(name, size_ret) != CL_SUCCESS)      \
        {                                                           \
            ERR() << "Failed to query CL device info for " << name; \
            return info;                                            \
        }                                                           \
    } while (0)

#define ANGLE_GET_INFO(name, size, param) \
    device->getDispatch().clGetDeviceInfo(device, name, size, param, nullptr)

#define ANGLE_GET_INFO_RET(name, size, param)                       \
    do                                                              \
    {                                                               \
        if (ANGLE_GET_INFO(name, size, param) != CL_SUCCESS)        \
        {                                                           \
            ERR() << "Failed to query CL device info for " << name; \
            return Info{};                                          \
        }                                                           \
    } while (0)

CLDeviceCL *CLDeviceCL::Create(cl_device_id device)
{
    size_t valueSize = 0u;
    if (ANGLE_GET_INFO_SIZE(CL_DEVICE_VERSION, &valueSize) == CL_SUCCESS)
    {
        std::vector<char> valString(valueSize, '\0');
        if (ANGLE_GET_INFO(CL_DEVICE_VERSION, valueSize, valString.data()) == CL_SUCCESS)
        {
            const cl_version version = ExtractCLVersion(valString.data());
            if (version != 0u)
            {
                return new CLDeviceCL(device, version);
            }
        }
    }
    return nullptr;
}

CLDeviceImpl::Info CLDeviceCL::GetInfo(cl_device_id device)
{
    Info info;
    size_t valueSize = 0u;
    std::vector<char> valString;

    if (ANGLE_GET_INFO_SIZE(CL_DEVICE_ILS_WITH_VERSION, &valueSize) == CL_SUCCESS &&
        (valueSize % sizeof(decltype(info.mILsWithVersion)::value_type)) == 0u)
    {
        info.mILsWithVersion.resize(valueSize / sizeof(decltype(info.mILsWithVersion)::value_type));
        ANGLE_GET_INFO_RET(CL_DEVICE_ILS_WITH_VERSION, valueSize, info.mILsWithVersion.data());
        info.mIsSupportedILsWithVersion = true;
    }

    if (ANGLE_GET_INFO_SIZE(CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION, &valueSize) == CL_SUCCESS &&
        (valueSize % sizeof(decltype(info.mBuiltInKernelsWithVersion)::value_type)) == 0u)
    {
        info.mBuiltInKernelsWithVersion.resize(
            valueSize / sizeof(decltype(info.mBuiltInKernelsWithVersion)::value_type));
        ANGLE_GET_INFO_RET(CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION, valueSize,
                           info.mBuiltInKernelsWithVersion.data());
        info.mIsSupportedBuiltInKernelsWithVersion = true;
    }

    if (ANGLE_GET_INFO_SIZE(CL_DEVICE_OPENCL_C_ALL_VERSIONS, &valueSize) == CL_SUCCESS &&
        (valueSize % sizeof(decltype(info.mOpenCL_C_AllVersions)::value_type)) == 0u)
    {
        info.mOpenCL_C_AllVersions.resize(valueSize /
                                          sizeof(decltype(info.mOpenCL_C_AllVersions)::value_type));
        ANGLE_GET_INFO_RET(CL_DEVICE_OPENCL_C_ALL_VERSIONS, valueSize,
                           info.mOpenCL_C_AllVersions.data());
        info.mIsSupportedOpenCL_C_AllVersions = true;
    }

    if (ANGLE_GET_INFO_SIZE(CL_DEVICE_OPENCL_C_FEATURES, &valueSize) == CL_SUCCESS &&
        (valueSize % sizeof(decltype(info.mOpenCL_C_Features)::value_type)) == 0u)
    {
        info.mOpenCL_C_Features.resize(valueSize /
                                       sizeof(decltype(info.mOpenCL_C_Features)::value_type));
        ANGLE_GET_INFO_RET(CL_DEVICE_OPENCL_C_FEATURES, valueSize, info.mOpenCL_C_Features.data());
        info.mIsSupportedOpenCL_C_Features = true;
    }

    ANGLE_GET_INFO_SIZE_RET(CL_DEVICE_EXTENSIONS, &valueSize);
    valString.resize(valueSize, '\0');
    ANGLE_GET_INFO_RET(CL_DEVICE_EXTENSIONS, valueSize, valString.data());
    info.mExtensions.assign(valString.data());
    RemoveUnsupportedCLExtensions(info.mExtensions);

    if (ANGLE_GET_INFO_SIZE(CL_DEVICE_EXTENSIONS_WITH_VERSION, &valueSize) == CL_SUCCESS &&
        (valueSize % sizeof(decltype(info.mExtensionsWithVersion)::value_type)) == 0u)
    {
        info.mExtensionsWithVersion.resize(
            valueSize / sizeof(decltype(info.mExtensionsWithVersion)::value_type));
        ANGLE_GET_INFO_RET(CL_DEVICE_EXTENSIONS_WITH_VERSION, valueSize,
                           info.mExtensionsWithVersion.data());
        RemoveUnsupportedCLExtensions(info.mExtensionsWithVersion);
        info.mIsSupportedExtensionsWithVersion = true;
    }

    if (ANGLE_GET_INFO_SIZE(CL_DEVICE_PARTITION_PROPERTIES, &valueSize) == CL_SUCCESS &&
        (valueSize % sizeof(decltype(info.mPartitionProperties)::value_type)) == 0u)
    {
        info.mPartitionProperties.resize(valueSize /
                                         sizeof(decltype(info.mPartitionProperties)::value_type));
        ANGLE_GET_INFO_RET(CL_DEVICE_PARTITION_PROPERTIES, valueSize,
                           info.mPartitionProperties.data());
    }

    if (ANGLE_GET_INFO_SIZE(CL_DEVICE_PARTITION_TYPE, &valueSize) == CL_SUCCESS &&
        (valueSize % sizeof(decltype(info.mPartitionType)::value_type)) == 0u)
    {
        info.mPartitionType.resize(valueSize / sizeof(decltype(info.mPartitionType)::value_type));
        ANGLE_GET_INFO_RET(CL_DEVICE_PARTITION_TYPE, valueSize, info.mPartitionType.data());
    }

    // Get this last, so the info is invalid if anything before fails
    cl_uint maxWorkItemDims = 0u;
    ANGLE_GET_INFO_RET(CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(maxWorkItemDims),
                       &maxWorkItemDims);
    info.mMaxWorkItemSizes.resize(maxWorkItemDims, 0u);
    ANGLE_GET_INFO_RET(CL_DEVICE_MAX_WORK_ITEM_SIZES,
                       maxWorkItemDims * sizeof(decltype(info.mMaxWorkItemSizes)::value_type),
                       info.mMaxWorkItemSizes.data());

    return info;
}

CLDeviceCL::CLDeviceCL(cl_device_id device, cl_version version) : mDevice(device), mVersion(version)
{}

}  // namespace rx
