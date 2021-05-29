//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLDevice.cpp: Implements the cl::Device class.

#include "libANGLE/CLDevice.h"

#include "libANGLE/CLPlatform.h"

#include <cstring>

namespace cl
{

Device::~Device() = default;

bool Device::supportsBuiltInKernel(const std::string &name) const
{
    if (name.empty() || mInfo.mBuiltInKernels.empty())
    {
        return false;
    }
    // Compare kernel name with all sub-strings terminated by semi-colon or end of string
    std::string::size_type start = 0u;
    do
    {
        std::string::size_type end = mInfo.mBuiltInKernels.find(';', start);
        if (end == std::string::npos)
        {
            end = mInfo.mBuiltInKernels.length();
        }
        const std::string::size_type length = end - start;
        if (length == name.length() && mInfo.mBuiltInKernels.compare(start, length, name) == 0)
        {
            return true;
        }
        start = end + 1u;
    } while (start < mInfo.mBuiltInKernels.size());
    return false;
}

cl_int Device::getInfo(DeviceInfo name, size_t valueSize, void *value, size_t *valueSizeRet) const
{
    static_assert(std::is_same<cl_uint, cl_bool>::value &&
                      std::is_same<cl_uint, cl_device_mem_cache_type>::value &&
                      std::is_same<cl_uint, cl_device_local_mem_type>::value &&
                      std::is_same<cl_uint, cl_version>::value &&
                      std::is_same<cl_ulong, cl_device_type>::value &&
                      std::is_same<cl_ulong, cl_device_fp_config>::value &&
                      std::is_same<cl_ulong, cl_device_exec_capabilities>::value &&
                      std::is_same<cl_ulong, cl_command_queue_properties>::value &&
                      std::is_same<cl_ulong, cl_device_affinity_domain>::value &&
                      std::is_same<cl_ulong, cl_device_svm_capabilities>::value &&
                      std::is_same<cl_ulong, cl_device_atomic_capabilities>::value &&
                      std::is_same<cl_ulong, cl_device_device_enqueue_capabilities>::value,
                  "OpenCL type mismatch");

    cl_uint valUInt   = 0u;
    cl_ulong valULong = 0u;
    size_t valSizeT   = 0u;
    void *valPointer  = nullptr;
    std::vector<char> valString;

    const void *copyValue = nullptr;
    size_t copySize       = 0u;
    cl_int result         = CL_SUCCESS;

    // The info names are sorted within their type group in the order they appear in the OpenCL
    // specification, so it is easier to compare them side-by-side when looking for changes.
    // https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clGetDeviceInfo
    switch (name)
    {
        // Handle all cl_uint and aliased types
        case DeviceInfo::VendorID:
        case DeviceInfo::MaxComputeUnits:
        case DeviceInfo::PreferredVectorWidthChar:
        case DeviceInfo::PreferredVectorWidthShort:
        case DeviceInfo::PreferredVectorWidthInt:
        case DeviceInfo::PreferredVectorWidthLong:
        case DeviceInfo::PreferredVectorWidthFloat:
        case DeviceInfo::PreferredVectorWidthDouble:
        case DeviceInfo::PreferredVectorWidthHalf:
        case DeviceInfo::NativeVectorWidthChar:
        case DeviceInfo::NativeVectorWidthShort:
        case DeviceInfo::NativeVectorWidthInt:
        case DeviceInfo::NativeVectorWidthLong:
        case DeviceInfo::NativeVectorWidthFloat:
        case DeviceInfo::NativeVectorWidthDouble:
        case DeviceInfo::NativeVectorWidthHalf:
        case DeviceInfo::MaxClockFrequency:
        case DeviceInfo::AddressBits:
        case DeviceInfo::MaxReadImageArgs:
        case DeviceInfo::MaxWriteImageArgs:
        case DeviceInfo::MaxReadWriteImageArgs:
        case DeviceInfo::MaxSamplers:
        case DeviceInfo::MaxPipeArgs:
        case DeviceInfo::PipeMaxActiveReservations:
        case DeviceInfo::PipeMaxPacketSize:
        case DeviceInfo::MemBaseAddrAlign:
        case DeviceInfo::MinDataTypeAlignSize:
        case DeviceInfo::GlobalMemCacheType:
        case DeviceInfo::GlobalMemCachelineSize:
        case DeviceInfo::MaxConstantArgs:
        case DeviceInfo::LocalMemType:
        case DeviceInfo::ErrorCorrectionSupport:
        case DeviceInfo::HostUnifiedMemory:
        case DeviceInfo::EndianLittle:
        case DeviceInfo::Available:
        case DeviceInfo::CompilerAvailable:
        case DeviceInfo::LinkerAvailable:
        case DeviceInfo::QueueOnDevicePreferredSize:
        case DeviceInfo::MaxOnDeviceQueues:
        case DeviceInfo::MaxOnDeviceEvents:
        case DeviceInfo::PreferredInteropUserSync:
        case DeviceInfo::PartitionMaxSubDevices:
        case DeviceInfo::PreferredPlatformAtomicAlignment:
        case DeviceInfo::PreferredGlobalAtomicAlignment:
        case DeviceInfo::PreferredLocalAtomicAlignment:
        case DeviceInfo::MaxNumSubGroups:
        case DeviceInfo::SubGroupIndependentForwardProgress:
        case DeviceInfo::NonUniformWorkGroupSupport:
        case DeviceInfo::WorkGroupCollectiveFunctionsSupport:
        case DeviceInfo::GenericAddressSpaceSupport:
        case DeviceInfo::PipeSupport:
            result    = mImpl->getInfoUInt(name, &valUInt);
            copyValue = &valUInt;
            copySize  = sizeof(valUInt);
            break;

        // Handle all cl_ulong and aliased types
        case DeviceInfo::SingleFpConfig:
        case DeviceInfo::DoubleFpConfig:
        case DeviceInfo::GlobalMemCacheSize:
        case DeviceInfo::GlobalMemSize:
        case DeviceInfo::MaxConstantBufferSize:
        case DeviceInfo::LocalMemSize:
        case DeviceInfo::ExecutionCapabilities:
        case DeviceInfo::QueueOnHostProperties:
        case DeviceInfo::QueueOnDeviceProperties:
        case DeviceInfo::PartitionAffinityDomain:
        case DeviceInfo::SVM_Capabilities:
        case DeviceInfo::AtomicMemoryCapabilities:
        case DeviceInfo::AtomicFenceCapabilities:
        case DeviceInfo::DeviceEnqueueCapabilities:
        case DeviceInfo::HalfFpConfig:
            result    = mImpl->getInfoULong(name, &valULong);
            copyValue = &valULong;
            copySize  = sizeof(valULong);
            break;

        // Handle all size_t and aliased types
        case DeviceInfo::MaxWorkGroupSize:
        case DeviceInfo::MaxParameterSize:
        case DeviceInfo::MaxGlobalVariableSize:
        case DeviceInfo::GlobalVariablePreferredTotalSize:
        case DeviceInfo::ProfilingTimerResolution:
        case DeviceInfo::PrintfBufferSize:
        case DeviceInfo::PreferredWorkGroupSizeMultiple:
            result    = mImpl->getInfoSizeT(name, &valSizeT);
            copyValue = &valSizeT;
            copySize  = sizeof(valSizeT);
            break;

        // Handle all string types
        case DeviceInfo::Name:
        case DeviceInfo::Vendor:
        case DeviceInfo::DriverVersion:
        case DeviceInfo::Profile:
        case DeviceInfo::OpenCL_C_Version:
        case DeviceInfo::LatestConformanceVersionPassed:
            result = mImpl->getInfoStringLength(name, &copySize);
            if (result != CL_SUCCESS)
            {
                return result;
            }
            valString.resize(copySize, '\0');
            result    = mImpl->getInfoString(name, copySize, valString.data());
            copyValue = valString.data();
            break;

        // Handle all cached values
        case DeviceInfo::Type:
            copyValue = &mInfo.mType;
            copySize  = sizeof(mInfo.mType);
            break;
        case DeviceInfo::MaxWorkItemDimensions:
            valUInt   = static_cast<cl_uint>(mInfo.mMaxWorkItemSizes.size());
            copyValue = &valUInt;
            copySize  = sizeof(valUInt);
            break;
        case DeviceInfo::MaxWorkItemSizes:
            copyValue = mInfo.mMaxWorkItemSizes.data();
            copySize  = mInfo.mMaxWorkItemSizes.size() *
                       sizeof(decltype(mInfo.mMaxWorkItemSizes)::value_type);
            break;
        case DeviceInfo::MaxMemAllocSize:
            copyValue = &mInfo.mMaxMemAllocSize;
            copySize  = sizeof(mInfo.mMaxMemAllocSize);
            break;
        case DeviceInfo::ImageSupport:
            copyValue = &mInfo.mImageSupport;
            copySize  = sizeof(mInfo.mImageSupport);
            break;
        case DeviceInfo::IL_Version:
            copyValue = mInfo.mIL_Version.c_str();
            copySize  = mInfo.mIL_Version.length() + 1u;
            break;
        case DeviceInfo::ILsWithVersion:
            copyValue = mInfo.mILsWithVersion.data();
            copySize =
                mInfo.mILsWithVersion.size() * sizeof(decltype(mInfo.mILsWithVersion)::value_type);
            break;
        case DeviceInfo::Image2D_MaxWidth:
            copyValue = &mInfo.mImage2D_MaxWidth;
            copySize  = sizeof(mInfo.mImage2D_MaxWidth);
            break;
        case DeviceInfo::Image2D_MaxHeight:
            copyValue = &mInfo.mImage2D_MaxHeight;
            copySize  = sizeof(mInfo.mImage2D_MaxHeight);
            break;
        case DeviceInfo::Image3D_MaxWidth:
            copyValue = &mInfo.mImage3D_MaxWidth;
            copySize  = sizeof(mInfo.mImage3D_MaxWidth);
            break;
        case DeviceInfo::Image3D_MaxHeight:
            copyValue = &mInfo.mImage3D_MaxHeight;
            copySize  = sizeof(mInfo.mImage3D_MaxHeight);
            break;
        case DeviceInfo::Image3D_MaxDepth:
            copyValue = &mInfo.mImage3D_MaxDepth;
            copySize  = sizeof(mInfo.mImage3D_MaxDepth);
            break;
        case DeviceInfo::ImageMaxBufferSize:
            copyValue = &mInfo.mImageMaxBufferSize;
            copySize  = sizeof(mInfo.mImageMaxBufferSize);
            break;
        case DeviceInfo::ImageMaxArraySize:
            copyValue = &mInfo.mImageMaxArraySize;
            copySize  = sizeof(mInfo.mImageMaxArraySize);
            break;
        case DeviceInfo::ImagePitchAlignment:
            copyValue = &mInfo.mImagePitchAlignment;
            copySize  = sizeof(mInfo.mImagePitchAlignment);
            break;
        case DeviceInfo::ImageBaseAddressAlignment:
            copyValue = &mInfo.mImageBaseAddressAlignment;
            copySize  = sizeof(mInfo.mImageBaseAddressAlignment);
            break;
        case DeviceInfo::QueueOnDeviceMaxSize:
            copyValue = &mInfo.mQueueOnDeviceMaxSize;
            copySize  = sizeof(mInfo.mQueueOnDeviceMaxSize);
            break;
        case DeviceInfo::BuiltInKernels:
            copyValue = mInfo.mBuiltInKernels.c_str();
            copySize  = mInfo.mBuiltInKernels.length() + 1u;
            break;
        case DeviceInfo::BuiltInKernelsWithVersion:
            copyValue = mInfo.mBuiltInKernelsWithVersion.data();
            copySize  = mInfo.mBuiltInKernelsWithVersion.size() *
                       sizeof(decltype(mInfo.mBuiltInKernelsWithVersion)::value_type);
            break;
        case DeviceInfo::Version:
            copyValue = mInfo.mVersionStr.c_str();
            copySize  = mInfo.mVersionStr.length() + 1u;
            break;
        case DeviceInfo::NumericVersion:
            copyValue = &mInfo.mVersion;
            copySize  = sizeof(mInfo.mVersion);
            break;
        case DeviceInfo::OpenCL_C_AllVersions:
            copyValue = mInfo.mOpenCL_C_AllVersions.data();
            copySize  = mInfo.mOpenCL_C_AllVersions.size() *
                       sizeof(decltype(mInfo.mOpenCL_C_AllVersions)::value_type);
            break;
        case DeviceInfo::OpenCL_C_Features:
            copyValue = mInfo.mOpenCL_C_Features.data();
            copySize  = mInfo.mOpenCL_C_Features.size() *
                       sizeof(decltype(mInfo.mOpenCL_C_Features)::value_type);
            break;
        case DeviceInfo::Extensions:
            copyValue = mInfo.mExtensions.c_str();
            copySize  = mInfo.mExtensions.length() + 1u;
            break;
        case DeviceInfo::ExtensionsWithVersion:
            copyValue = mInfo.mExtensionsWithVersion.data();
            copySize  = mInfo.mExtensionsWithVersion.size() *
                       sizeof(decltype(mInfo.mExtensionsWithVersion)::value_type);
            break;
        case DeviceInfo::PartitionProperties:
            copyValue = mInfo.mPartitionProperties.data();
            copySize  = mInfo.mPartitionProperties.size() *
                       sizeof(decltype(mInfo.mPartitionProperties)::value_type);
            break;
        case DeviceInfo::PartitionType:
            copyValue = mInfo.mPartitionType.data();
            copySize =
                mInfo.mPartitionType.size() * sizeof(decltype(mInfo.mPartitionType)::value_type);
            break;

        // Handle all mapped values
        case DeviceInfo::Platform:
            valPointer = mPlatform.getNative();
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case DeviceInfo::ParentDevice:
            valPointer = mParent->getNative();
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case DeviceInfo::ReferenceCount:
            valUInt   = isRoot() ? 1u : getRefCount();
            copyValue = &valUInt;
            copySize  = sizeof(valUInt);
            break;

        default:
            ASSERT(false);
            return CL_INVALID_VALUE;
    }

    if (result != CL_SUCCESS)
    {
        return result;
    }
    if (value != nullptr)
    {
        // CL_INVALID_VALUE if size in bytes specified by param_value_size is < size of return
        // type as specified in the Device Queries table and param_value is not a NULL value
        if (valueSize < copySize)
        {
            return CL_INVALID_VALUE;
        }
        if (copyValue != nullptr)
        {
            std::memcpy(value, copyValue, copySize);
        }
    }
    if (valueSizeRet != nullptr)
    {
        *valueSizeRet = copySize;
    }
    return CL_SUCCESS;
}

cl_int Device::createSubDevices(const cl_device_partition_property *properties,
                                cl_uint numDevices,
                                cl_device_id *subDevices,
                                cl_uint *numDevicesRet)
{
    if (subDevices == nullptr)
    {
        numDevices = 0u;
    }
    rx::CLDeviceImpl::CreateFuncs subDeviceCreateFuncs;
    const cl_int errorCode =
        mImpl->createSubDevices(properties, numDevices, subDeviceCreateFuncs, numDevicesRet);
    if (errorCode == CL_SUCCESS)
    {
        cl::DeviceType type = mInfo.mType;
        type.clear(CL_DEVICE_TYPE_DEFAULT);
        DevicePtrs devices;
        devices.reserve(subDeviceCreateFuncs.size());
        while (!subDeviceCreateFuncs.empty())
        {
            devices.emplace_back(new Device(mPlatform, this, type, subDeviceCreateFuncs.front()));
            if (!devices.back()->mInfo.isValid())
            {
                return CL_INVALID_VALUE;
            }
            subDeviceCreateFuncs.pop_front();
        }
        for (DevicePtr &subDevice : devices)
        {
            *subDevices++ = subDevice.release();
        }
    }
    return errorCode;
}

Device::Device(Platform &platform,
               Device *parent,
               DeviceType type,
               const rx::CLDeviceImpl::CreateFunc &createFunc)
    : mPlatform(platform), mParent(parent), mImpl(createFunc(*this)), mInfo(mImpl->createInfo(type))
{}

}  // namespace cl
