//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLDeviceCL.cpp: Implements the class methods for CLDeviceCL.

#include "libANGLE/renderer/cl/CLDeviceCL.h"

#include "libANGLE/Debug.h"

namespace rx
{

CLDeviceCL::CLDeviceCL(cl_device_id device) : mDevice(device) {}

CLDeviceCL::~CLDeviceCL() = default;

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

#define ANGLE_GET_INFO_SIZE(name, size_ret) \
    device->getDispatch().clGetDeviceInfo(device, name, 0u, nullptr, size_ret)

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

CLDeviceImpl::Info CLDeviceCL::GetInfo(cl_device_id device)
{
    Info info;
    size_t valueSize = 0u;

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

    if (ANGLE_GET_INFO_SIZE(CL_DEVICE_EXTENSIONS_WITH_VERSION, &valueSize) == CL_SUCCESS &&
        (valueSize % sizeof(decltype(info.mExtensionsWithVersion)::value_type)) == 0u)
    {
        info.mExtensionsWithVersion.resize(
            valueSize / sizeof(decltype(info.mExtensionsWithVersion)::value_type));
        ANGLE_GET_INFO_RET(CL_DEVICE_EXTENSIONS_WITH_VERSION, valueSize,
                           info.mExtensionsWithVersion.data());
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

}  // namespace rx
