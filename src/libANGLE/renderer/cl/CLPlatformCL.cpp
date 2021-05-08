//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatformCL.cpp: Implements the class methods for CLPlatformCL.

#include "libANGLE/renderer/cl/CLPlatformCL.h"

#include "libANGLE/renderer/cl/CLDeviceCL.h"
#include "libANGLE/renderer/cl/cl_util.h"

#include "libANGLE/CLPlatform.h"
#include "libANGLE/Debug.h"

#include "anglebase/no_destructor.h"
#include "common/angle_version.h"

extern "C" {
#include "icd.h"
}  // extern "C"

namespace rx
{

CLPlatformCL::~CLPlatformCL() = default;

CLDeviceImpl::InitList CLPlatformCL::getDevices()
{
    CLDeviceImpl::InitList initList;

    // Fetch all regular devices. This does not include CL_DEVICE_TYPE_CUSTOM, which are not
    // supported by the CL pass-through back end because they have no standard feature set.
    // This makes them unreliable for the purpose of this back end.
    cl_uint numDevices = 0u;
    if (mPlatform->getDispatch().clGetDeviceIDs(mPlatform, CL_DEVICE_TYPE_ALL, 0u, nullptr,
                                                &numDevices) == CL_SUCCESS)
    {
        std::vector<cl_device_id> devices(numDevices, nullptr);
        if (mPlatform->getDispatch().clGetDeviceIDs(mPlatform, CL_DEVICE_TYPE_ALL, numDevices,
                                                    devices.data(), nullptr) == CL_SUCCESS)
        {
            for (cl_device_id device : devices)
            {
                CLDeviceImpl::Ptr impl(CLDeviceCL::Create(device));
                CLDeviceImpl::Info info = CLDeviceCL::GetInfo(device);
                if (impl && info.isValid())
                {
                    initList.emplace_back(std::move(impl), std::move(info));
                }
            }
        }
        else
        {
            ERR() << "Failed to query CL devices";
        }
    }
    else
    {
        ERR() << "Failed to query CL devices";
    }

    return initList;
}

CLPlatformCL::InitList CLPlatformCL::GetPlatforms(bool isIcd)
{
    // Using khrIcdInitialize() of the third party Khronos OpenCL ICD Loader to enumerate the
    // available OpenCL implementations on the system. They will be stored in the singly linked
    // list khrIcdVendors of the C struct KHRicdVendor.
    if (khrIcdVendors == nullptr)
    {
        // Our OpenCL entry points are not reentrant, so we have to prevent khrIcdInitialize()
        // from querying ANGLE's OpenCL library. We store a dummy entry with the library in the
        // khrIcdVendors list, because the ICD Loader skips the libraries which are already in
        // the list as it assumes they were already enumerated.
        static angle::base::NoDestructor<KHRicdVendor> sVendorAngle({});
        sVendorAngle->library = khrIcdOsLibraryLoad(ANGLE_OPENCL_LIB_NAME);
        khrIcdVendors         = sVendorAngle.get();
        if (khrIcdVendors->library != nullptr)
        {
            khrIcdInitialize();
            // After the enumeration we don't need ANGLE's OpenCL library any more,
            // but we keep the dummy entry int the list to prevent another enumeration.
            khrIcdOsLibraryUnload(khrIcdVendors->library);
            khrIcdVendors->library = nullptr;
        }
    }

    // Iterating through the singly linked list khrIcdVendors to create an ANGLE CL pass-through
    // platform for each found ICD platform. Skipping our dummy entry that has an invalid platform.
    InitList initList;
    for (KHRicdVendor *vendorIt = khrIcdVendors; vendorIt != nullptr; vendorIt = vendorIt->next)
    {
        if (vendorIt->platform != nullptr)
        {
            Info info = GetInfo(vendorIt->platform);
            if (info.isValid())
            {
                initList.emplace_back(new CLPlatformCL(vendorIt->platform), std::move(info));
            }
        }
    }
    return initList;
}

CLPlatformCL::CLPlatformCL(cl_platform_id platform) : mPlatform(platform) {}

#define ANGLE_GET_INFO_SIZE(name, size_ret) \
    platform->getDispatch().clGetPlatformInfo(platform, name, 0u, nullptr, size_ret)

#define ANGLE_GET_INFO_SIZE_RET(name, size_ret)                       \
    do                                                                \
    {                                                                 \
        if (ANGLE_GET_INFO_SIZE(name, size_ret) != CL_SUCCESS)        \
        {                                                             \
            ERR() << "Failed to query CL platform info for " << name; \
            return info;                                              \
        }                                                             \
    } while (0)

#define ANGLE_GET_INFO(name, size, param) \
    platform->getDispatch().clGetPlatformInfo(platform, name, size, param, nullptr)

#define ANGLE_GET_INFO_RET(name, size, param)                         \
    do                                                                \
    {                                                                 \
        if (ANGLE_GET_INFO(name, size, param) != CL_SUCCESS)          \
        {                                                             \
            ERR() << "Failed to query CL platform info for " << name; \
            return info;                                              \
        }                                                             \
    } while (0)

CLPlatformImpl::Info CLPlatformCL::GetInfo(cl_platform_id platform)
{
    Info info;
    size_t valueSize = 0u;
    std::vector<char> valString;

    // Verify that the platform is valid
    if (platform == nullptr || platform->getDispatch().clGetPlatformIDs == nullptr ||
        platform->getDispatch().clGetPlatformInfo == nullptr ||
        platform->getDispatch().clGetDeviceIDs == nullptr ||
        platform->getDispatch().clGetDeviceInfo == nullptr ||
        platform->getDispatch().clCreateContext == nullptr ||
        platform->getDispatch().clCreateContextFromType == nullptr ||
        platform->getDispatch().clRetainContext == nullptr ||
        platform->getDispatch().clReleaseContext == nullptr ||
        platform->getDispatch().clGetContextInfo == nullptr ||
        platform->getDispatch().clCreateCommandQueue == nullptr ||
        platform->getDispatch().clRetainCommandQueue == nullptr ||
        platform->getDispatch().clReleaseCommandQueue == nullptr ||
        platform->getDispatch().clGetCommandQueueInfo == nullptr ||
        platform->getDispatch().clSetCommandQueueProperty == nullptr ||
        platform->getDispatch().clCreateBuffer == nullptr ||
        platform->getDispatch().clCreateImage2D == nullptr ||
        platform->getDispatch().clCreateImage3D == nullptr ||
        platform->getDispatch().clRetainMemObject == nullptr ||
        platform->getDispatch().clReleaseMemObject == nullptr ||
        platform->getDispatch().clGetSupportedImageFormats == nullptr ||
        platform->getDispatch().clGetMemObjectInfo == nullptr ||
        platform->getDispatch().clGetImageInfo == nullptr ||
        platform->getDispatch().clCreateSampler == nullptr ||
        platform->getDispatch().clRetainSampler == nullptr ||
        platform->getDispatch().clReleaseSampler == nullptr ||
        platform->getDispatch().clGetSamplerInfo == nullptr ||
        platform->getDispatch().clCreateProgramWithSource == nullptr ||
        platform->getDispatch().clCreateProgramWithBinary == nullptr ||
        platform->getDispatch().clRetainProgram == nullptr ||
        platform->getDispatch().clReleaseProgram == nullptr ||
        platform->getDispatch().clBuildProgram == nullptr ||
        platform->getDispatch().clUnloadCompiler == nullptr ||
        platform->getDispatch().clGetProgramInfo == nullptr ||
        platform->getDispatch().clGetProgramBuildInfo == nullptr ||
        platform->getDispatch().clCreateKernel == nullptr ||
        platform->getDispatch().clCreateKernelsInProgram == nullptr ||
        platform->getDispatch().clRetainKernel == nullptr ||
        platform->getDispatch().clReleaseKernel == nullptr ||
        platform->getDispatch().clSetKernelArg == nullptr ||
        platform->getDispatch().clGetKernelInfo == nullptr ||
        platform->getDispatch().clGetKernelWorkGroupInfo == nullptr ||
        platform->getDispatch().clWaitForEvents == nullptr ||
        platform->getDispatch().clGetEventInfo == nullptr ||
        platform->getDispatch().clRetainEvent == nullptr ||
        platform->getDispatch().clReleaseEvent == nullptr ||
        platform->getDispatch().clGetEventProfilingInfo == nullptr ||
        platform->getDispatch().clFlush == nullptr || platform->getDispatch().clFinish == nullptr ||
        platform->getDispatch().clEnqueueReadBuffer == nullptr ||
        platform->getDispatch().clEnqueueWriteBuffer == nullptr ||
        platform->getDispatch().clEnqueueCopyBuffer == nullptr ||
        platform->getDispatch().clEnqueueReadImage == nullptr ||
        platform->getDispatch().clEnqueueWriteImage == nullptr ||
        platform->getDispatch().clEnqueueCopyImage == nullptr ||
        platform->getDispatch().clEnqueueCopyImageToBuffer == nullptr ||
        platform->getDispatch().clEnqueueCopyBufferToImage == nullptr ||
        platform->getDispatch().clEnqueueMapBuffer == nullptr ||
        platform->getDispatch().clEnqueueMapImage == nullptr ||
        platform->getDispatch().clEnqueueUnmapMemObject == nullptr ||
        platform->getDispatch().clEnqueueNDRangeKernel == nullptr ||
        platform->getDispatch().clEnqueueTask == nullptr ||
        platform->getDispatch().clEnqueueNativeKernel == nullptr ||
        platform->getDispatch().clEnqueueMarker == nullptr ||
        platform->getDispatch().clEnqueueWaitForEvents == nullptr ||
        platform->getDispatch().clEnqueueBarrier == nullptr ||
        platform->getDispatch().clGetExtensionFunctionAddress == nullptr)
    {
        ERR() << "Missing entry points for OpenCL 1.0";
        return info;
    }

    // Skip ANGLE CL implementation to prevent passthrough loop
    ANGLE_GET_INFO_SIZE_RET(CL_PLATFORM_VENDOR, &valueSize);
    valString.resize(valueSize, '\0');
    ANGLE_GET_INFO_RET(CL_PLATFORM_VENDOR, valueSize, valString.data());
    if (std::string(valString.data()).compare(cl::Platform::GetVendor()) == 0)
    {
        ERR() << "Tried to create CL pass-through back end for ANGLE library";
        return info;
    }

    // Skip platform if it is not ICD compatible
    ANGLE_GET_INFO_SIZE_RET(CL_PLATFORM_EXTENSIONS, &valueSize);
    valString.resize(valueSize, '\0');
    ANGLE_GET_INFO_RET(CL_PLATFORM_EXTENSIONS, valueSize, valString.data());
    info.mExtensions.assign(valString.data());
    RemoveUnsupportedCLExtensions(info.mExtensions);
    if (info.mExtensions.find("cl_khr_icd") == std::string::npos)
    {
        WARN() << "CL platform is not ICD compatible";
        return info;
    }

    // Fetch common platform info
    ANGLE_GET_INFO_SIZE_RET(CL_PLATFORM_VERSION, &valueSize);
    valString.resize(valueSize, '\0');
    ANGLE_GET_INFO_RET(CL_PLATFORM_VERSION, valueSize, valString.data());
    info.mVersionStr.assign(valString.data());
    info.mVersionStr += " (ANGLE " ANGLE_VERSION_STRING ")";

    const cl_version version = ExtractCLVersion(info.mVersionStr);
    if (version == 0u)
    {
        return info;
    }

    if (ANGLE_GET_INFO(CL_PLATFORM_NUMERIC_VERSION, sizeof(info.mVersion), &info.mVersion) !=
        CL_SUCCESS)
    {
        info.mVersion = version;
    }
    else if (CL_VERSION_MAJOR(info.mVersion) != CL_VERSION_MAJOR(version) ||
             CL_VERSION_MINOR(info.mVersion) != CL_VERSION_MINOR(version))
    {
        WARN() << "CL_PLATFORM_NUMERIC_VERSION = " << CL_VERSION_MAJOR(info.mVersion) << '.'
               << CL_VERSION_MINOR(info.mVersion)
               << " does not match version string: " << info.mVersionStr;
    }

    ANGLE_GET_INFO_SIZE_RET(CL_PLATFORM_NAME, &valueSize);
    valString.resize(valueSize, '\0');
    ANGLE_GET_INFO_RET(CL_PLATFORM_NAME, valueSize, valString.data());
    info.mName.assign("ANGLE pass-through -> ");
    info.mName += valString.data();

    if (ANGLE_GET_INFO_SIZE(CL_PLATFORM_EXTENSIONS_WITH_VERSION, &valueSize) == CL_SUCCESS &&
        (valueSize % sizeof(decltype(info.mExtensionsWithVersion)::value_type)) == 0u)
    {
        info.mExtensionsWithVersion.resize(
            valueSize / sizeof(decltype(info.mExtensionsWithVersion)::value_type));
        ANGLE_GET_INFO_RET(CL_PLATFORM_EXTENSIONS_WITH_VERSION, valueSize,
                           info.mExtensionsWithVersion.data());
        RemoveUnsupportedCLExtensions(info.mExtensionsWithVersion);
    }

    ANGLE_GET_INFO(CL_PLATFORM_HOST_TIMER_RESOLUTION, sizeof(info.mHostTimerRes),
                   &info.mHostTimerRes);

    if (info.mVersion >= CL_MAKE_VERSION(1, 1, 0) &&
        (platform->getDispatch().clSetEventCallback == nullptr ||
         platform->getDispatch().clCreateSubBuffer == nullptr ||
         platform->getDispatch().clSetMemObjectDestructorCallback == nullptr ||
         platform->getDispatch().clCreateUserEvent == nullptr ||
         platform->getDispatch().clSetUserEventStatus == nullptr ||
         platform->getDispatch().clEnqueueReadBufferRect == nullptr ||
         platform->getDispatch().clEnqueueWriteBufferRect == nullptr ||
         platform->getDispatch().clEnqueueCopyBufferRect == nullptr))
    {
        ERR() << "Missing entry points for OpenCL 1.1";
        return info;
    }

    if (info.mVersion >= CL_MAKE_VERSION(1, 2, 0) &&
        (platform->getDispatch().clCreateSubDevices == nullptr ||
         platform->getDispatch().clRetainDevice == nullptr ||
         platform->getDispatch().clReleaseDevice == nullptr ||
         platform->getDispatch().clCreateImage == nullptr ||
         platform->getDispatch().clCreateProgramWithBuiltInKernels == nullptr ||
         platform->getDispatch().clCompileProgram == nullptr ||
         platform->getDispatch().clLinkProgram == nullptr ||
         platform->getDispatch().clUnloadPlatformCompiler == nullptr ||
         platform->getDispatch().clGetKernelArgInfo == nullptr ||
         platform->getDispatch().clEnqueueFillBuffer == nullptr ||
         platform->getDispatch().clEnqueueFillImage == nullptr ||
         platform->getDispatch().clEnqueueMigrateMemObjects == nullptr ||
         platform->getDispatch().clEnqueueMarkerWithWaitList == nullptr ||
         platform->getDispatch().clEnqueueBarrierWithWaitList == nullptr ||
         platform->getDispatch().clGetExtensionFunctionAddressForPlatform == nullptr))
    {
        ERR() << "Missing entry points for OpenCL 1.2";
        return info;
    }

    if (info.mVersion >= CL_MAKE_VERSION(2, 0, 0) &&
        (platform->getDispatch().clCreateCommandQueueWithProperties == nullptr ||
         platform->getDispatch().clCreatePipe == nullptr ||
         platform->getDispatch().clGetPipeInfo == nullptr ||
         platform->getDispatch().clSVMAlloc == nullptr ||
         platform->getDispatch().clSVMFree == nullptr ||
         platform->getDispatch().clEnqueueSVMFree == nullptr ||
         platform->getDispatch().clEnqueueSVMMemcpy == nullptr ||
         platform->getDispatch().clEnqueueSVMMemFill == nullptr ||
         platform->getDispatch().clEnqueueSVMMap == nullptr ||
         platform->getDispatch().clEnqueueSVMUnmap == nullptr ||
         platform->getDispatch().clCreateSamplerWithProperties == nullptr ||
         platform->getDispatch().clSetKernelArgSVMPointer == nullptr ||
         platform->getDispatch().clSetKernelExecInfo == nullptr))
    {
        ERR() << "Missing entry points for OpenCL 2.0";
        return info;
    }

    if (info.mVersion >= CL_MAKE_VERSION(2, 1, 0) &&
        (platform->getDispatch().clCloneKernel == nullptr ||
         platform->getDispatch().clCreateProgramWithIL == nullptr ||
         platform->getDispatch().clEnqueueSVMMigrateMem == nullptr ||
         platform->getDispatch().clGetDeviceAndHostTimer == nullptr ||
         platform->getDispatch().clGetHostTimer == nullptr ||
         platform->getDispatch().clGetKernelSubGroupInfo == nullptr ||
         platform->getDispatch().clSetDefaultDeviceCommandQueue == nullptr))
    {
        ERR() << "Missing entry points for OpenCL 2.1";
        return info;
    }

    if (info.mVersion >= CL_MAKE_VERSION(2, 2, 0) &&
        (platform->getDispatch().clSetProgramReleaseCallback == nullptr ||
         platform->getDispatch().clSetProgramSpecializationConstant == nullptr))
    {
        ERR() << "Missing entry points for OpenCL 2.2";
        return info;
    }

    if (info.mVersion >= CL_MAKE_VERSION(3, 0, 0) &&
        (platform->getDispatch().clCreateBufferWithProperties == nullptr ||
         platform->getDispatch().clCreateImageWithProperties == nullptr ||
         platform->getDispatch().clSetContextDestructorCallback == nullptr))
    {
        ERR() << "Missing entry points for OpenCL 3.0";
        return info;
    }

    // Get this last, so the info is invalid if anything before fails
    ANGLE_GET_INFO_SIZE_RET(CL_PLATFORM_PROFILE, &valueSize);
    valString.resize(valueSize, '\0');
    ANGLE_GET_INFO_RET(CL_PLATFORM_PROFILE, valueSize, valString.data());
    info.mProfile.assign(valString.data());

    return info;
}

}  // namespace rx
