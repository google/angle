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
#include "common/system_utils.h"

extern "C" {
#include "icd.h"
}  // extern "C"

namespace rx
{

namespace
{

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

CLPlatformImpl::Info CLPlatformCL::createInfo() const
{
    // Verify that the platform is valid
    if (mNative == nullptr || mNative->getDispatch().clGetPlatformIDs == nullptr ||
        mNative->getDispatch().clGetPlatformInfo == nullptr ||
        mNative->getDispatch().clGetDeviceIDs == nullptr ||
        mNative->getDispatch().clGetDeviceInfo == nullptr ||
        mNative->getDispatch().clCreateContext == nullptr ||
        mNative->getDispatch().clCreateContextFromType == nullptr ||
        mNative->getDispatch().clRetainContext == nullptr ||
        mNative->getDispatch().clReleaseContext == nullptr ||
        mNative->getDispatch().clGetContextInfo == nullptr ||
        mNative->getDispatch().clCreateCommandQueue == nullptr ||
        mNative->getDispatch().clRetainCommandQueue == nullptr ||
        mNative->getDispatch().clReleaseCommandQueue == nullptr ||
        mNative->getDispatch().clGetCommandQueueInfo == nullptr ||
        mNative->getDispatch().clSetCommandQueueProperty == nullptr ||
        mNative->getDispatch().clCreateBuffer == nullptr ||
        mNative->getDispatch().clCreateImage2D == nullptr ||
        mNative->getDispatch().clCreateImage3D == nullptr ||
        mNative->getDispatch().clRetainMemObject == nullptr ||
        mNative->getDispatch().clReleaseMemObject == nullptr ||
        mNative->getDispatch().clGetSupportedImageFormats == nullptr ||
        mNative->getDispatch().clGetMemObjectInfo == nullptr ||
        mNative->getDispatch().clGetImageInfo == nullptr ||
        mNative->getDispatch().clCreateSampler == nullptr ||
        mNative->getDispatch().clRetainSampler == nullptr ||
        mNative->getDispatch().clReleaseSampler == nullptr ||
        mNative->getDispatch().clGetSamplerInfo == nullptr ||
        mNative->getDispatch().clCreateProgramWithSource == nullptr ||
        mNative->getDispatch().clCreateProgramWithBinary == nullptr ||
        mNative->getDispatch().clRetainProgram == nullptr ||
        mNative->getDispatch().clReleaseProgram == nullptr ||
        mNative->getDispatch().clBuildProgram == nullptr ||
        mNative->getDispatch().clUnloadCompiler == nullptr ||
        mNative->getDispatch().clGetProgramInfo == nullptr ||
        mNative->getDispatch().clGetProgramBuildInfo == nullptr ||
        mNative->getDispatch().clCreateKernel == nullptr ||
        mNative->getDispatch().clCreateKernelsInProgram == nullptr ||
        mNative->getDispatch().clRetainKernel == nullptr ||
        mNative->getDispatch().clReleaseKernel == nullptr ||
        mNative->getDispatch().clSetKernelArg == nullptr ||
        mNative->getDispatch().clGetKernelInfo == nullptr ||
        mNative->getDispatch().clGetKernelWorkGroupInfo == nullptr ||
        mNative->getDispatch().clWaitForEvents == nullptr ||
        mNative->getDispatch().clGetEventInfo == nullptr ||
        mNative->getDispatch().clRetainEvent == nullptr ||
        mNative->getDispatch().clReleaseEvent == nullptr ||
        mNative->getDispatch().clGetEventProfilingInfo == nullptr ||
        mNative->getDispatch().clFlush == nullptr || mNative->getDispatch().clFinish == nullptr ||
        mNative->getDispatch().clEnqueueReadBuffer == nullptr ||
        mNative->getDispatch().clEnqueueWriteBuffer == nullptr ||
        mNative->getDispatch().clEnqueueCopyBuffer == nullptr ||
        mNative->getDispatch().clEnqueueReadImage == nullptr ||
        mNative->getDispatch().clEnqueueWriteImage == nullptr ||
        mNative->getDispatch().clEnqueueCopyImage == nullptr ||
        mNative->getDispatch().clEnqueueCopyImageToBuffer == nullptr ||
        mNative->getDispatch().clEnqueueCopyBufferToImage == nullptr ||
        mNative->getDispatch().clEnqueueMapBuffer == nullptr ||
        mNative->getDispatch().clEnqueueMapImage == nullptr ||
        mNative->getDispatch().clEnqueueUnmapMemObject == nullptr ||
        mNative->getDispatch().clEnqueueNDRangeKernel == nullptr ||
        mNative->getDispatch().clEnqueueTask == nullptr ||
        mNative->getDispatch().clEnqueueNativeKernel == nullptr ||
        mNative->getDispatch().clEnqueueMarker == nullptr ||
        mNative->getDispatch().clEnqueueWaitForEvents == nullptr ||
        mNative->getDispatch().clEnqueueBarrier == nullptr ||
        mNative->getDispatch().clGetExtensionFunctionAddress == nullptr)
    {
        ERR() << "Missing entry points for OpenCL 1.0";
        return Info{};
    }

    // Fetch common platform info
    Info info;
    const std::string vendor = GetPlatformString(mNative, cl::PlatformInfo::Vendor);
    info.mProfile            = GetPlatformString(mNative, cl::PlatformInfo::Profile);
    info.mVersionStr         = GetPlatformString(mNative, cl::PlatformInfo::Version);
    info.mName               = GetPlatformString(mNative, cl::PlatformInfo::Name);
    info.mExtensions         = GetPlatformString(mNative, cl::PlatformInfo::Extensions);

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
        mNative->getDispatch().clGetPlatformInfo(mNative, CL_PLATFORM_HOST_TIMER_RESOLUTION,
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
        if (mNative->getDispatch().clGetPlatformInfo(mNative, CL_PLATFORM_NUMERIC_VERSION,
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
        if (mNative->getDispatch().clGetPlatformInfo(mNative, CL_PLATFORM_EXTENSIONS_WITH_VERSION,
                                                     0u, nullptr, &valueSize) != CL_SUCCESS ||
            (valueSize % sizeof(decltype(info.mExtensionsWithVersion)::value_type)) != 0u)
        {
            ERR() << "Failed to query CL platform info for CL_PLATFORM_EXTENSIONS_WITH_VERSION";
            return Info{};
        }
        info.mExtensionsWithVersion.resize(
            valueSize / sizeof(decltype(info.mExtensionsWithVersion)::value_type));
        if (mNative->getDispatch().clGetPlatformInfo(mNative, CL_PLATFORM_EXTENSIONS_WITH_VERSION,
                                                     valueSize, info.mExtensionsWithVersion.data(),
                                                     nullptr) != CL_SUCCESS)
        {
            ERR() << "Failed to query CL platform info for CL_PLATFORM_EXTENSIONS_WITH_VERSION";
            return Info{};
        }
        RemoveUnsupportedCLExtensions(info.mExtensionsWithVersion);
    }

    if (info.mVersion >= CL_MAKE_VERSION(1, 1, 0) &&
        (mNative->getDispatch().clSetEventCallback == nullptr ||
         mNative->getDispatch().clCreateSubBuffer == nullptr ||
         mNative->getDispatch().clSetMemObjectDestructorCallback == nullptr ||
         mNative->getDispatch().clCreateUserEvent == nullptr ||
         mNative->getDispatch().clSetUserEventStatus == nullptr ||
         mNative->getDispatch().clEnqueueReadBufferRect == nullptr ||
         mNative->getDispatch().clEnqueueWriteBufferRect == nullptr ||
         mNative->getDispatch().clEnqueueCopyBufferRect == nullptr))
    {
        ERR() << "Missing entry points for OpenCL 1.1";
        return Info{};
    }

    if (info.mVersion >= CL_MAKE_VERSION(1, 2, 0) &&
        (mNative->getDispatch().clCreateSubDevices == nullptr ||
         mNative->getDispatch().clRetainDevice == nullptr ||
         mNative->getDispatch().clReleaseDevice == nullptr ||
         mNative->getDispatch().clCreateImage == nullptr ||
         mNative->getDispatch().clCreateProgramWithBuiltInKernels == nullptr ||
         mNative->getDispatch().clCompileProgram == nullptr ||
         mNative->getDispatch().clLinkProgram == nullptr ||
         mNative->getDispatch().clUnloadPlatformCompiler == nullptr ||
         mNative->getDispatch().clGetKernelArgInfo == nullptr ||
         mNative->getDispatch().clEnqueueFillBuffer == nullptr ||
         mNative->getDispatch().clEnqueueFillImage == nullptr ||
         mNative->getDispatch().clEnqueueMigrateMemObjects == nullptr ||
         mNative->getDispatch().clEnqueueMarkerWithWaitList == nullptr ||
         mNative->getDispatch().clEnqueueBarrierWithWaitList == nullptr ||
         mNative->getDispatch().clGetExtensionFunctionAddressForPlatform == nullptr))
    {
        ERR() << "Missing entry points for OpenCL 1.2";
        return Info{};
    }

    if (info.mVersion >= CL_MAKE_VERSION(2, 0, 0) &&
        (mNative->getDispatch().clCreateCommandQueueWithProperties == nullptr ||
         mNative->getDispatch().clCreatePipe == nullptr ||
         mNative->getDispatch().clGetPipeInfo == nullptr ||
         mNative->getDispatch().clSVMAlloc == nullptr ||
         mNative->getDispatch().clSVMFree == nullptr ||
         mNative->getDispatch().clEnqueueSVMFree == nullptr ||
         mNative->getDispatch().clEnqueueSVMMemcpy == nullptr ||
         mNative->getDispatch().clEnqueueSVMMemFill == nullptr ||
         mNative->getDispatch().clEnqueueSVMMap == nullptr ||
         mNative->getDispatch().clEnqueueSVMUnmap == nullptr ||
         mNative->getDispatch().clCreateSamplerWithProperties == nullptr ||
         mNative->getDispatch().clSetKernelArgSVMPointer == nullptr ||
         mNative->getDispatch().clSetKernelExecInfo == nullptr))
    {
        ERR() << "Missing entry points for OpenCL 2.0";
        return Info{};
    }

    if (info.mVersion >= CL_MAKE_VERSION(2, 1, 0) &&
        (mNative->getDispatch().clCloneKernel == nullptr ||
         mNative->getDispatch().clCreateProgramWithIL == nullptr ||
         mNative->getDispatch().clEnqueueSVMMigrateMem == nullptr ||
         mNative->getDispatch().clGetDeviceAndHostTimer == nullptr ||
         mNative->getDispatch().clGetHostTimer == nullptr ||
         mNative->getDispatch().clGetKernelSubGroupInfo == nullptr ||
         mNative->getDispatch().clSetDefaultDeviceCommandQueue == nullptr))
    {
        ERR() << "Missing entry points for OpenCL 2.1";
        return Info{};
    }

    if (info.mVersion >= CL_MAKE_VERSION(2, 2, 0) &&
        (mNative->getDispatch().clSetProgramReleaseCallback == nullptr ||
         mNative->getDispatch().clSetProgramSpecializationConstant == nullptr))
    {
        ERR() << "Missing entry points for OpenCL 2.2";
        return Info{};
    }

    if (info.mVersion >= CL_MAKE_VERSION(3, 0, 0) &&
        (mNative->getDispatch().clCreateBufferWithProperties == nullptr ||
         mNative->getDispatch().clCreateImageWithProperties == nullptr ||
         mNative->getDispatch().clSetContextDestructorCallback == nullptr))
    {
        ERR() << "Missing entry points for OpenCL 3.0";
        return Info{};
    }

    return info;
}

cl::DevicePtrList CLPlatformCL::createDevices(cl::Platform &platform) const
{
    cl::DevicePtrList devices;

    // Fetch all regular devices. This does not include CL_DEVICE_TYPE_CUSTOM, which are not
    // supported by the CL pass-through back end because they have no standard feature set.
    // This makes them unreliable for the purpose of this back end.
    cl_uint numDevices = 0u;
    if (mNative->getDispatch().clGetDeviceIDs(mNative, CL_DEVICE_TYPE_ALL, 0u, nullptr,
                                              &numDevices) == CL_SUCCESS)
    {
        std::vector<cl_device_id> nativeDevices(numDevices, nullptr);
        if (mNative->getDispatch().clGetDeviceIDs(mNative, CL_DEVICE_TYPE_ALL, numDevices,
                                                  nativeDevices.data(), nullptr) == CL_SUCCESS)
        {
            // Fetch all device types for front end initialization, and find the default device.
            // If none exists declare first device as default.
            std::vector<cl_device_type> types(nativeDevices.size(), 0u);
            size_t defaultIndex = 0u;
            for (size_t index = 0u; index < nativeDevices.size(); ++index)
            {
                if (nativeDevices[index]->getDispatch().clGetDeviceInfo(
                        nativeDevices[index], CL_DEVICE_TYPE, sizeof(cl_device_type), &types[index],
                        nullptr) == CL_SUCCESS)
                {
                    // If default device found, select it
                    if ((types[index] & CL_DEVICE_TYPE_DEFAULT) != 0u)
                    {
                        defaultIndex = index;
                    }
                }
                else
                {
                    types.clear();
                    nativeDevices.clear();
                }
            }

            for (size_t index = 0u; index < nativeDevices.size(); ++index)
            {
                // Make sure the default bit is set in exactly one device
                if (index == defaultIndex)
                {
                    types[index] |= CL_DEVICE_TYPE_DEFAULT;
                }
                else
                {
                    types[index] &= ~CL_DEVICE_TYPE_DEFAULT;
                }

                const cl::Device::CreateImplFunc createImplFunc = [&](const cl::Device &device) {
                    return CLDeviceCL::Ptr(new CLDeviceCL(device, nativeDevices[index]));
                };
                devices.emplace_back(
                    cl::Device::CreateDevice(platform, nullptr, types[index], createImplFunc));
                if (!devices.back())
                {
                    devices.clear();
                    break;
                }
            }
        }
    }

    if (devices.empty())
    {
        ERR() << "Failed to query CL devices";
    }
    return devices;
}

CLContextImpl::Ptr CLPlatformCL::createContext(const cl::Context &context,
                                               const cl::DeviceRefList &devices,
                                               cl::ContextErrorCB notify,
                                               void *userData,
                                               bool userSync,
                                               cl_int *errcodeRet)
{
    cl_context_properties properties[] = {
        CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(mNative),
        userSync && mPlatform.isVersionOrNewer(1u, 2u) ? CL_CONTEXT_INTEROP_USER_SYNC : 0, CL_TRUE,
        0};

    std::vector<cl_device_id> nativeDevices;
    for (const cl::DeviceRefPtr &device : devices)
    {
        nativeDevices.emplace_back(device->getImpl<CLDeviceCL &>().getNative());
    }

    CLContextImpl::Ptr contextImpl;
    cl_context nativeContext = mNative->getDispatch().clCreateContext(
        properties, static_cast<cl_uint>(nativeDevices.size()), nativeDevices.data(), notify,
        userData, errcodeRet);
    return CLContextImpl::Ptr(nativeContext != nullptr ? new CLContextCL(context, nativeContext)
                                                       : nullptr);
}

CLContextImpl::Ptr CLPlatformCL::createContextFromType(const cl::Context &context,
                                                       cl_device_type deviceType,
                                                       cl::ContextErrorCB notify,
                                                       void *userData,
                                                       bool userSync,
                                                       cl_int *errcodeRet)
{
    cl_context_properties properties[] = {
        CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(mNative),
        userSync && mPlatform.isVersionOrNewer(1u, 2u) ? CL_CONTEXT_INTEROP_USER_SYNC : 0, CL_TRUE,
        0};
    cl_context nativeContext = mNative->getDispatch().clCreateContextFromType(
        properties, deviceType, notify, userData, errcodeRet);
    return CLContextImpl::Ptr(nativeContext != nullptr ? new CLContextCL(context, nativeContext)
                                                       : nullptr);
}

void CLPlatformCL::Initialize(const cl_icd_dispatch &dispatch, bool isIcd)
{
    // Using khrIcdInitialize() of the third party Khronos OpenCL ICD Loader to enumerate the
    // available OpenCL implementations on the system. They will be stored in the singly linked
    // list khrIcdVendors of the C struct KHRicdVendor.
    if (khrIcdVendors != nullptr)
    {
        return;
    }

    // The absolute path to ANGLE's OpenCL library is needed and it is assumed here that
    // it is in the same directory as the shared library which contains this CL back end.
    std::string libPath = angle::GetModuleDirectory();
    if (!libPath.empty() && libPath.back() != angle::GetPathSeparator())
    {
        libPath += angle::GetPathSeparator();
    }
    libPath += ANGLE_OPENCL_LIB_NAME;
    libPath += '.';
    libPath += angle::GetSharedLibraryExtension();

    // Our OpenCL entry points are not reentrant, so we have to prevent khrIcdInitialize()
    // from querying ANGLE's OpenCL library. We store a dummy entry with the library in the
    // khrIcdVendors list, because the ICD Loader skips the libraries which are already in
    // the list as it assumes they were already enumerated.
    static angle::base::NoDestructor<KHRicdVendor> sVendorAngle({});
    sVendorAngle->library = khrIcdOsLibraryLoad(libPath.c_str());
    khrIcdVendors         = sVendorAngle.get();
    if (khrIcdVendors->library == nullptr)
    {
        WARN() << "Unable to load library \"" << libPath << "\"";
        return;
    }

    khrIcdInitialize();
    // After the enumeration we don't need ANGLE's OpenCL library any more,
    // but we keep the dummy entry int the list to prevent another enumeration.
    khrIcdOsLibraryUnload(khrIcdVendors->library);
    khrIcdVendors->library = nullptr;

    // Iterating through the singly linked list khrIcdVendors to create an ANGLE CL pass-through
    // platform for each found ICD platform. Skipping our dummy entry that has an invalid platform.
    for (KHRicdVendor *vendorIt = khrIcdVendors; vendorIt != nullptr; vendorIt = vendorIt->next)
    {
        if (vendorIt->platform != nullptr)
        {
            const cl::Platform::CreateImplFunc createImplFunc = [&](const cl::Platform &platform) {
                return Ptr(new CLPlatformCL(platform, vendorIt->platform));
            };
            cl::Platform::CreatePlatform(dispatch, createImplFunc);
        }
    }
}

CLPlatformCL::CLPlatformCL(const cl::Platform &platform, cl_platform_id native)
    : CLPlatformImpl(platform), mNative(native)
{}

}  // namespace rx
