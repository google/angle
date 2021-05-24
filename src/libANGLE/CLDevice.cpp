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

Device::~Device()
{
    if (isRoot())
    {
        removeRef();
    }
}

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

bool Device::release()
{
    if (isRoot())
    {
        return false;
    }
    const bool released = removeRef();
    if (released)
    {
        mParent->destroySubDevice(this);
    }
    return released;
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
        case DeviceInfo::ImageSupport:
        case DeviceInfo::MaxReadImageArgs:
        case DeviceInfo::MaxWriteImageArgs:
        case DeviceInfo::MaxReadWriteImageArgs:
        case DeviceInfo::MaxSamplers:
        case DeviceInfo::ImagePitchAlignment:
        case DeviceInfo::ImageBaseAddressAlignment:
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
        case DeviceInfo::QueueOnDeviceMaxSize:
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
        case DeviceInfo::Image2D_MaxWidth:
        case DeviceInfo::Image2D_MaxHeight:
        case DeviceInfo::Image3D_MaxWidth:
        case DeviceInfo::Image3D_MaxHeight:
        case DeviceInfo::Image3D_MaxDepth:
        case DeviceInfo::ImageMaxBufferSize:
        case DeviceInfo::ImageMaxArraySize:
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
        case DeviceInfo::IL_Version:
            copyValue = mInfo.mIL_Version.c_str();
            copySize  = mInfo.mIL_Version.length() + 1u;
            break;
        case DeviceInfo::ILsWithVersion:
            copyValue = mInfo.mILsWithVersion.data();
            copySize =
                mInfo.mILsWithVersion.size() * sizeof(decltype(mInfo.mILsWithVersion)::value_type);
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
            valPointer = static_cast<cl_platform_id>(&mPlatform);
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case DeviceInfo::ParentDevice:
            valPointer = static_cast<cl_device_id>(mParent.get());
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case DeviceInfo::ReferenceCount:
            copyValue = getRefCountPtr();
            copySize  = sizeof(*getRefCountPtr());
            break;

        default:
            WARN() << "CL device info " << name << " is not (yet) supported";
            return CL_INVALID_VALUE;
    }

    if (result != CL_SUCCESS)
    {
        return result;
    }
    if (value != nullptr)
    {
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
    DevicePtrList subDeviceList;
    const cl_int result =
        mImpl->createSubDevices(*this, properties, numDevices, subDeviceList, numDevicesRet);
    if (result == CL_SUCCESS)
    {
        for (const DevicePtr &subDevice : subDeviceList)
        {
            *subDevices++ = subDevice.get();
        }
        mSubDevices.splice(mSubDevices.cend(), std::move(subDeviceList));
    }
    return result;
}

DevicePtr Device::CreateDevice(Platform &platform,
                               Device *parent,
                               cl_device_type type,
                               const CreateImplFunc &createImplFunc)
{
    DevicePtr device(new Device(platform, parent, type, createImplFunc));
    return device->mInfo.isValid() ? std::move(device) : DevicePtr{};
}

bool Device::IsValid(const _cl_device_id *device)
{
    const Platform::PtrList &platforms = Platform::GetPlatforms();
    return std::find_if(platforms.cbegin(), platforms.cend(), [=](const PlatformPtr &platform) {
               return platform->hasDevice(device);
           }) != platforms.cend();
}

Device::Device(Platform &platform,
               Device *parent,
               cl_device_type type,
               const CreateImplFunc &createImplFunc)
    : _cl_device_id(platform.getDispatch()),
      mPlatform(platform),
      mParent(parent),
      mImpl(createImplFunc(*this)),
      mInfo(mImpl->createInfo(type))
{}

void Device::destroySubDevice(Device *device)
{
    auto deviceIt = mSubDevices.cbegin();
    while (deviceIt != mSubDevices.cend() && deviceIt->get() != device)
    {
        ++deviceIt;
    }
    if (deviceIt != mSubDevices.cend())
    {
        mSubDevices.erase(deviceIt);
    }
    else
    {
        ERR() << "Sub-device not found";
    }
}

}  // namespace cl
