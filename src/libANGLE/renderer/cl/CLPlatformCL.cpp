//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatformCL.cpp: Implements the class methods for CLPlatformCL.

#include "libANGLE/renderer/cl/CLPlatformCL.h"

#include "libANGLE/renderer/cl/CLContextCL.h"
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

namespace
{

CLDeviceImpl::List CreateDevices(CLPlatformCL &platform,
                                 cl_platform_id native,
                                 CLDeviceImpl::PtrList &implPtrList)
{
    CLDeviceImpl::List implList;

    // Fetch all regular devices. This does not include CL_DEVICE_TYPE_CUSTOM, which are not
    // supported by the CL pass-through back end because they have no standard feature set.
    // This makes them unreliable for the purpose of this back end.
    cl_uint numDevices = 0u;
    if (native->getDispatch().clGetDeviceIDs(native, CL_DEVICE_TYPE_ALL, 0u, nullptr,
                                             &numDevices) == CL_SUCCESS)
    {
        std::vector<cl_device_id> devices(numDevices, nullptr);
        if (native->getDispatch().clGetDeviceIDs(native, CL_DEVICE_TYPE_ALL, numDevices,
                                                 devices.data(), nullptr) == CL_SUCCESS)
        {
            for (cl_device_id device : devices)
            {
                CLDeviceImpl::Ptr impl(CLDeviceCL::Create(platform, nullptr, device));
                if (!impl)
                {
                    implList.clear();
                    implPtrList.clear();
                    break;
                }
                implList.emplace_back(impl.get());
                implPtrList.emplace_back(std::move(impl));
            }
        }
    }

    if (implList.empty())
    {
        ERR() << "Failed to query CL devices";
    }
    return implList;
}

std::string GetPlatformString(cl_platform_id platform, cl::PlatformInfo name)
{
    size_t size = 0u;
    if (platform->getDispatch().clGetPlatformInfo(platform, cl::ToCLenum(name), 0u, nullptr,
                                                  &size) == CL_SUCCESS)
    {
        std::vector<char> str(size, '\0');
        if (platform->getDispatch().clGetPlatformInfo(platform, cl::ToCLenum(name), size,
                                                      str.data(), nullptr) == CL_SUCCESS)
        {
            return std::string(str.data());
        }
    }
    ERR() << "Failed to query CL platform info for " << name;
    return std::string{};
}

}  // namespace

CLPlatformCL::~CLPlatformCL() = default;

CLContextImpl::Ptr CLPlatformCL::createContext(CLDeviceImpl::List &&deviceImplList,
                                               cl::ContextErrorCB notify,
                                               void *userData,
                                               bool userSync,
                                               cl_int *errcodeRet)
{
    cl_context_properties properties[] = {
        CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(mPlatform),
        userSync && mVersion >= CL_MAKE_VERSION(1, 2, 0) ? CL_CONTEXT_INTEROP_USER_SYNC : 0,
        CL_TRUE, 0};

    std::vector<cl_device_id> devices;
    for (CLDeviceImpl *deviceImpl : deviceImplList)
    {
        devices.emplace_back(static_cast<CLDeviceCL *>(deviceImpl)->getNative());
    }

    CLContextImpl::Ptr contextImpl;
    cl_context context =
        mPlatform->getDispatch().clCreateContext(properties, static_cast<cl_uint>(devices.size()),
                                                 devices.data(), notify, userData, errcodeRet);
    if (context != nullptr)
    {
        contextImpl.reset(new CLContextCL(*this, std::move(deviceImplList), context));
        mContexts.emplace_back(contextImpl.get());
    }
    return contextImpl;
}

CLContextImpl::Ptr CLPlatformCL::createContextFromType(cl_device_type deviceType,
                                                       cl::ContextErrorCB notify,
                                                       void *userData,
                                                       bool userSync,
                                                       cl_int *errcodeRet)
{
    cl_context_properties properties[] = {
        CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(mPlatform),
        userSync && mVersion >= CL_MAKE_VERSION(1, 2, 0) ? CL_CONTEXT_INTEROP_USER_SYNC : 0,
        CL_TRUE, 0};
    cl_context context = mPlatform->getDispatch().clCreateContextFromType(
        properties, deviceType, notify, userData, errcodeRet);
    if (context == nullptr)
    {
        return CLContextImpl::Ptr{};
    }

    size_t valueSize = 0u;
    cl_int result    = context->getDispatch().clGetContextInfo(context, CL_CONTEXT_DEVICES, 0u,
                                                            nullptr, &valueSize);
    if (result == CL_SUCCESS && (valueSize % sizeof(cl_device_id)) == 0u)
    {
        std::vector<cl_device_id> devices(valueSize / sizeof(cl_device_id), nullptr);
        result = context->getDispatch().clGetContextInfo(context, CL_CONTEXT_DEVICES, valueSize,
                                                         devices.data(), nullptr);
        if (result == CL_SUCCESS)
        {
            CLDeviceImpl::List deviceImplList;
            for (cl_device_id device : devices)
            {
                auto it = mDevices.cbegin();
                while (it != mDevices.cend() &&
                       static_cast<CLDeviceCL *>(*it)->getNative() != device)
                {
                    ++it;
                }
                if (it != mDevices.cend())
                {
                    deviceImplList.emplace_back(*it);
                }
                else
                {
                    ERR() << "Device not found in platform list";
                }
            }
            if (deviceImplList.size() == devices.size())
            {
                CLContextImpl::Ptr contextImpl(
                    new CLContextCL(*this, std::move(deviceImplList), context));
                mContexts.emplace_back(contextImpl.get());
                return contextImpl;
            }
            result = CL_INVALID_VALUE;
        }
    }

    context->getDispatch().clReleaseContext(context);
    if (errcodeRet != nullptr)
    {
        *errcodeRet = result;
    }
    return CLContextImpl::Ptr{};
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
                CLDeviceImpl::PtrList devices;
                Ptr platform(new CLPlatformCL(vendorIt->platform, info.mVersion, devices));
                if (!devices.empty())
                {
                    initList.emplace_back(std::move(platform), std::move(info), std::move(devices));
                }
            }
        }
    }
    return initList;
}

CLPlatformCL::CLPlatformCL(cl_platform_id platform,
                           cl_version version,
                           CLDeviceImpl::PtrList &devices)
    : CLPlatformImpl(CreateDevices(*this, platform, devices)),
      mPlatform(platform),
      mVersion(version)
{}

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
        return Info{};
    }

    // Fetch common platform info
    Info info;
    const std::string vendor = GetPlatformString(platform, cl::PlatformInfo::Vendor);
    info.mProfile            = GetPlatformString(platform, cl::PlatformInfo::Profile);
    info.mVersionStr         = GetPlatformString(platform, cl::PlatformInfo::Version);
    info.mName               = GetPlatformString(platform, cl::PlatformInfo::Name);
    info.mExtensions         = GetPlatformString(platform, cl::PlatformInfo::Extensions);

    if (vendor.empty() || info.mProfile.empty() || info.mVersionStr.empty() || info.mName.empty() ||
        info.mExtensions.empty())
    {
        return Info{};
    }

    // Skip ANGLE CL implementation to prevent passthrough loop
    if (vendor.compare(cl::Platform::GetVendor()) == 0)
    {
        ERR() << "Tried to create CL pass-through back end for ANGLE library";
        return Info{};
    }

    // Skip platform if it is not ICD compatible
    if (info.mExtensions.find("cl_khr_icd") == std::string::npos)
    {
        WARN() << "CL platform is not ICD compatible";
        return Info{};
    }

    const cl_version version = ExtractCLVersion(info.mVersionStr);
    if (version == 0u)
    {
        return Info{};
    }

    // Customize version string and name, and remove unsupported extensions
    info.mVersionStr += " (ANGLE " ANGLE_VERSION_STRING ")";
    info.mName.insert(0u, "ANGLE pass-through -> ");
    RemoveUnsupportedCLExtensions(info.mExtensions);

    if (version >= CL_MAKE_VERSION(2, 1, 0) &&
        platform->getDispatch().clGetPlatformInfo(platform, CL_PLATFORM_HOST_TIMER_RESOLUTION,
                                                  sizeof(info.mHostTimerRes), &info.mHostTimerRes,
                                                  nullptr) != CL_SUCCESS)
    {
        ERR() << "Failed to query CL platform info for CL_PLATFORM_HOST_TIMER_RESOLUTION";
        return Info{};
    }

    if (version < CL_MAKE_VERSION(3, 0, 0))
    {
        info.mVersion = version;
    }
    else
    {
        if (platform->getDispatch().clGetPlatformInfo(platform, CL_PLATFORM_NUMERIC_VERSION,
                                                      sizeof(info.mVersion), &info.mVersion,
                                                      nullptr) != CL_SUCCESS)
        {
            ERR() << "Failed to query CL platform info for CL_PLATFORM_NUMERIC_VERSION";
            return Info{};
        }
        else if (CL_VERSION_MAJOR(info.mVersion) != CL_VERSION_MAJOR(version) ||
                 CL_VERSION_MINOR(info.mVersion) != CL_VERSION_MINOR(version))
        {
            WARN() << "CL_PLATFORM_NUMERIC_VERSION = " << CL_VERSION_MAJOR(info.mVersion) << '.'
                   << CL_VERSION_MINOR(info.mVersion)
                   << " does not match version string: " << info.mVersionStr;
        }

        size_t valueSize = 0u;
        if (platform->getDispatch().clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS_WITH_VERSION,
                                                      0u, nullptr, &valueSize) != CL_SUCCESS ||
            (valueSize % sizeof(decltype(info.mExtensionsWithVersion)::value_type)) != 0u)
        {
            ERR() << "Failed to query CL platform info for CL_PLATFORM_EXTENSIONS_WITH_VERSION";
            return Info{};
        }
        info.mExtensionsWithVersion.resize(
            valueSize / sizeof(decltype(info.mExtensionsWithVersion)::value_type));
        if (platform->getDispatch().clGetPlatformInfo(platform, CL_PLATFORM_EXTENSIONS_WITH_VERSION,
                                                      valueSize, info.mExtensionsWithVersion.data(),
                                                      nullptr) != CL_SUCCESS)
        {
            ERR() << "Failed to query CL platform info for CL_PLATFORM_EXTENSIONS_WITH_VERSION";
            return Info{};
        }
        RemoveUnsupportedCLExtensions(info.mExtensionsWithVersion);
    }

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

    return info;
}

}  // namespace rx
