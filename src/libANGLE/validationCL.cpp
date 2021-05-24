//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// validationCL.cpp: Validation functions for generic CL entry point parameters
// based on the OpenCL Specificaion V3.0.7, see https://www.khronos.org/registry/OpenCL/

#include "libANGLE/validationCL_autogen.h"

#include "libANGLE/cl_utils.h"

#define ANGLE_ERROR_RETURN(error, ret) \
    if (errcode_ret != nullptr)        \
    {                                  \
        *errcode_ret = error;          \
    }                                  \
    return ret

namespace cl
{

namespace
{

const Platform *ValidateContextProperties(const cl_context_properties *properties,
                                          cl_int *errcode_ret)
{
    cl_platform_id platform = nullptr;
    bool hasUserSync        = false;
    if (properties != nullptr)
    {
        while (*properties != 0)
        {
            switch (*properties++)
            {
                case CL_CONTEXT_PLATFORM:
                    if (platform != nullptr)
                    {
                        ANGLE_ERROR_RETURN(CL_INVALID_PROPERTY, nullptr);
                    }
                    platform = reinterpret_cast<cl_platform_id>(*properties++);
                    if (!Platform::IsValid(platform))
                    {
                        ANGLE_ERROR_RETURN(CL_INVALID_PLATFORM, nullptr);
                    }
                    break;
                case CL_CONTEXT_INTEROP_USER_SYNC:
                    if (hasUserSync || (*properties != CL_FALSE && *properties != CL_TRUE))
                    {
                        ANGLE_ERROR_RETURN(CL_INVALID_PROPERTY, nullptr);
                    }
                    ++properties;
                    hasUserSync = true;
                    break;
                default:
                    ANGLE_ERROR_RETURN(CL_INVALID_PROPERTY, nullptr);
            }
        }
    }
    if (platform == nullptr)
    {
        platform = Platform::GetDefault();
        if (platform == nullptr)
        {
            ANGLE_ERROR_RETURN(CL_INVALID_PLATFORM, nullptr);
        }
    }
    return static_cast<Platform *>(platform);
}

bool ValidateMemoryFlags(cl_mem_flags flags, const Platform &platform)
{
    cl_mem_flags allowedFlags = CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY | CL_MEM_READ_ONLY;
    // These flags are mutually exclusive. Count of set flags should not exceed one.
    if (static_cast<int>((flags & CL_MEM_READ_WRITE) != 0u) +
            static_cast<int>((flags & CL_MEM_WRITE_ONLY) != 0u) +
            static_cast<int>((flags & CL_MEM_READ_ONLY) != 0u) >
        1)
    {
        return false;
    }

    allowedFlags |= CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR | CL_MEM_COPY_HOST_PTR;
    // CL_MEM_USE_HOST_PTR is mutually exclusive with the other two flags
    if ((flags & CL_MEM_USE_HOST_PTR) != 0u &&
        ((flags & CL_MEM_ALLOC_HOST_PTR) != 0u || (flags & CL_MEM_COPY_HOST_PTR) != 0u))
    {
        return false;
    }

    if (platform.isVersionOrNewer(1u, 2u))
    {
        allowedFlags |= CL_MEM_HOST_WRITE_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_HOST_NO_ACCESS;
        // These flags are mutually exclusive. Count of set flags should not exceed one.
        if (static_cast<int>((flags & CL_MEM_HOST_WRITE_ONLY) != 0u) +
                static_cast<int>((flags & CL_MEM_HOST_READ_ONLY) != 0u) +
                static_cast<int>((flags & CL_MEM_HOST_NO_ACCESS) != 0u) >
            1)
        {
            return false;
        }
    }

    if (platform.isVersionOrNewer(2u, 0u))
    {
        allowedFlags |= CL_MEM_KERNEL_READ_AND_WRITE;
    }

    // flags should be zero when allowed flags are masked out
    if ((flags & ~allowedFlags) != 0u)
    {
        return false;
    }

    return true;
}

bool ValidateMemoryProperties(const cl_mem_properties *properties)
{
    if (properties != nullptr)
    {
        // OpenCL 3.0 does not define any optional properties
        if (*properties != 0)
        {
            return false;
        }
    }
    return true;
}

bool ValidateImageFormat(const cl_image_format *image_format, const Platform &platform)
{
    if (image_format == nullptr)
    {
        return false;
    }
    switch (image_format->image_channel_order)
    {
        case CL_R:
        case CL_A:
        case CL_LUMINANCE:
        case CL_INTENSITY:
        case CL_RG:
        case CL_RA:
        case CL_RGB:
        case CL_RGBA:
        case CL_ARGB:
        case CL_BGRA:
            break;

        case CL_Rx:
        case CL_RGx:
        case CL_RGBx:
            if (!platform.isVersionOrNewer(1u, 1u))
            {
                return false;
            }
            break;

        case CL_DEPTH:
        case CL_ABGR:
        case CL_sRGB:
        case CL_sRGBA:
        case CL_sBGRA:
        case CL_sRGBx:
            if (!platform.isVersionOrNewer(2u, 0u))
            {
                return false;
            }
            break;

        default:
            return false;
    }

    switch (image_format->image_channel_data_type)
    {
        case CL_SNORM_INT8:
        case CL_SNORM_INT16:
        case CL_UNORM_INT8:
        case CL_UNORM_INT16:
        case CL_SIGNED_INT8:
        case CL_SIGNED_INT16:
        case CL_SIGNED_INT32:
        case CL_UNSIGNED_INT8:
        case CL_UNSIGNED_INT16:
        case CL_UNSIGNED_INT32:
        case CL_HALF_FLOAT:
        case CL_FLOAT:
            break;

        case CL_UNORM_SHORT_565:
        case CL_UNORM_SHORT_555:
        case CL_UNORM_INT_101010:
            if (image_format->image_channel_order != CL_RGB &&
                image_format->image_channel_order != CL_RGBx)
            {
                return false;
            }
            break;

        case CL_UNORM_INT_101010_2:
            if (!platform.isVersionOrNewer(2u, 1u) || image_format->image_channel_order != CL_RGBA)
            {
                return false;
            }
            break;

        default:
            return false;
    }
    return true;
}

}  // namespace

// CL 1.0
cl_int ValidateGetPlatformIDs(cl_uint num_entries,
                              const cl_platform_id *platforms,
                              const cl_uint *num_platforms)
{
    if ((num_entries == 0u && platforms != nullptr) ||
        (platforms == nullptr && num_platforms == nullptr))
    {
        return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

cl_int ValidateGetPlatformInfo(cl_platform_id platform,
                               PlatformInfo param_name,
                               size_t param_value_size,
                               const void *param_value,
                               const size_t *param_value_size_ret)
{
    if (!Platform::IsValidOrDefault(platform))
    {
        return CL_INVALID_PLATFORM;
    }
    const Platform &pltfrm = *static_cast<const Platform *>(platform);

    if (param_name == PlatformInfo::InvalidEnum ||
        (param_name == PlatformInfo::HostTimerResolution && !pltfrm.isVersionOrNewer(2u, 1u)) ||
        (param_name == PlatformInfo::NumericVersion && !pltfrm.isVersionOrNewer(3u, 0u)) ||
        (param_name == PlatformInfo::ExtensionsWithVersion && !pltfrm.isVersionOrNewer(3u, 0u)) ||
        (param_value_size == 0u && param_value != nullptr))
    {
        return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

cl_int ValidateGetDeviceIDs(cl_platform_id platform,
                            cl_device_type device_type,
                            cl_uint num_entries,
                            const cl_device_id *devices,
                            const cl_uint *num_devices)
{
    if (!Platform::IsValidOrDefault(platform))
    {
        return CL_INVALID_PLATFORM;
    }
    if (!Device::IsValidType(device_type))
    {
        return CL_INVALID_DEVICE_TYPE;
    }
    if ((num_entries == 0u && devices != nullptr) || (devices == nullptr && num_devices == nullptr))
    {
        return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

cl_int ValidateGetDeviceInfo(cl_device_id device,
                             DeviceInfo param_name,
                             size_t param_value_size,
                             const void *param_value,
                             const size_t *param_value_size_ret)
{
    if (!Device::IsValid(device))
    {
        return CL_INVALID_DEVICE;
    }
    if (param_name == DeviceInfo::InvalidEnum || (param_value_size == 0u && param_value != nullptr))
    {
        return CL_INVALID_VALUE;
    }
    const Device &dev = *static_cast<const Device *>(device);
    // Enums ordered within their version block as they appear in the OpenCL spec V3.0.7, table 5
    switch (param_name)
    {
        case DeviceInfo::PreferredVectorWidthHalf:
        case DeviceInfo::NativeVectorWidthChar:
        case DeviceInfo::NativeVectorWidthShort:
        case DeviceInfo::NativeVectorWidthInt:
        case DeviceInfo::NativeVectorWidthLong:
        case DeviceInfo::NativeVectorWidthFloat:
        case DeviceInfo::NativeVectorWidthDouble:
        case DeviceInfo::NativeVectorWidthHalf:
        case DeviceInfo::HostUnifiedMemory:
        case DeviceInfo::OpenCL_C_Version:
            if (!dev.isVersionOrNewer(1u, 1u))
            {
                return CL_INVALID_VALUE;
            }
            break;

        case DeviceInfo::ImageMaxBufferSize:
        case DeviceInfo::ImageMaxArraySize:
        case DeviceInfo::DoubleFpConfig:
        case DeviceInfo::LinkerAvailable:
        case DeviceInfo::BuiltInKernels:
        case DeviceInfo::PrintfBufferSize:
        case DeviceInfo::PreferredInteropUserSync:
        case DeviceInfo::ParentDevice:
        case DeviceInfo::PartitionMaxSubDevices:
        case DeviceInfo::PartitionProperties:
        case DeviceInfo::PartitionAffinityDomain:
        case DeviceInfo::PartitionType:
        case DeviceInfo::ReferenceCount:
            if (!dev.isVersionOrNewer(1u, 2u))
            {
                return CL_INVALID_VALUE;
            }
            break;

        case DeviceInfo::MaxReadWriteImageArgs:
        case DeviceInfo::ImagePitchAlignment:
        case DeviceInfo::ImageBaseAddressAlignment:
        case DeviceInfo::MaxPipeArgs:
        case DeviceInfo::PipeMaxActiveReservations:
        case DeviceInfo::PipeMaxPacketSize:
        case DeviceInfo::MaxGlobalVariableSize:
        case DeviceInfo::GlobalVariablePreferredTotalSize:
        case DeviceInfo::QueueOnDeviceProperties:
        case DeviceInfo::QueueOnDevicePreferredSize:
        case DeviceInfo::QueueOnDeviceMaxSize:
        case DeviceInfo::MaxOnDeviceQueues:
        case DeviceInfo::MaxOnDeviceEvents:
        case DeviceInfo::SVM_Capabilities:
        case DeviceInfo::PreferredPlatformAtomicAlignment:
        case DeviceInfo::PreferredGlobalAtomicAlignment:
        case DeviceInfo::PreferredLocalAtomicAlignment:
            if (!dev.isVersionOrNewer(2u, 0u))
            {
                return CL_INVALID_VALUE;
            }
            break;

        case DeviceInfo::IL_Version:
        case DeviceInfo::MaxNumSubGroups:
        case DeviceInfo::SubGroupIndependentForwardProgress:
            if (!dev.isVersionOrNewer(2u, 1u))
            {
                return CL_INVALID_VALUE;
            }
            break;

        case DeviceInfo::ILsWithVersion:
        case DeviceInfo::BuiltInKernelsWithVersion:
        case DeviceInfo::NumericVersion:
        case DeviceInfo::OpenCL_C_AllVersions:
        case DeviceInfo::OpenCL_C_Features:
        case DeviceInfo::ExtensionsWithVersion:
        case DeviceInfo::AtomicMemoryCapabilities:
        case DeviceInfo::AtomicFenceCapabilities:
        case DeviceInfo::NonUniformWorkGroupSupport:
        case DeviceInfo::WorkGroupCollectiveFunctionsSupport:
        case DeviceInfo::GenericAddressSpaceSupport:
        case DeviceInfo::DeviceEnqueueCapabilities:
        case DeviceInfo::PipeSupport:
        case DeviceInfo::PreferredWorkGroupSizeMultiple:
        case DeviceInfo::LatestConformanceVersionPassed:
            if (!dev.isVersionOrNewer(3u, 0u))
            {
                return CL_INVALID_VALUE;
            }
            break;

        case DeviceInfo::InvalidEnum:
            return CL_INVALID_VALUE;
        default:
            break;
    }
    return CL_SUCCESS;
}

bool ValidateCreateContext(const cl_context_properties *properties,
                           cl_uint num_devices,
                           const cl_device_id *devices,
                           void(CL_CALLBACK *pfn_notify)(const char *errinfo,
                                                         const void *private_info,
                                                         size_t cb,
                                                         void *user_data),
                           const void *user_data,
                           cl_int *errcode_ret)
{
    const Platform *const platform = ValidateContextProperties(properties, errcode_ret);
    if (platform == nullptr)
    {
        return false;
    }

    if (num_devices == 0u || devices == nullptr || (pfn_notify == nullptr && user_data != nullptr))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
    }
    while (num_devices-- > 0u)
    {
        if (!platform->hasDevice(*devices++))
        {
            ANGLE_ERROR_RETURN(CL_INVALID_DEVICE, false);
        }
    }
    return true;
}

bool ValidateCreateContextFromType(const cl_context_properties *properties,
                                   cl_device_type device_type,
                                   void(CL_CALLBACK *pfn_notify)(const char *errinfo,
                                                                 const void *private_info,
                                                                 size_t cb,
                                                                 void *user_data),
                                   const void *user_data,
                                   cl_int *errcode_ret)
{
    const Platform *const platform = ValidateContextProperties(properties, errcode_ret);
    if (platform == nullptr)
    {
        return false;
    }
    if (!Device::IsValidType(device_type))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_DEVICE_TYPE, false);
    }
    if (pfn_notify == nullptr && user_data != nullptr)
    {
        ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
    }
    return true;
}

cl_int ValidateRetainContext(cl_context context)
{
    return Context::IsValid(context) ? CL_SUCCESS : CL_INVALID_CONTEXT;
}

cl_int ValidateReleaseContext(cl_context context)
{
    return Context::IsValid(context) ? CL_SUCCESS : CL_INVALID_CONTEXT;
}

cl_int ValidateGetContextInfo(cl_context context,
                              ContextInfo param_name,
                              size_t param_value_size,
                              const void *param_value,
                              const size_t *param_value_size_ret)
{
    if (!Context::IsValid(context))
    {
        return CL_INVALID_CONTEXT;
    }
    if (param_name == ContextInfo::InvalidEnum ||
        (param_value_size == 0u && param_value != nullptr))
    {
        return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

cl_int ValidateRetainCommandQueue(cl_command_queue command_queue)
{
    return CommandQueue::IsValid(command_queue) ? CL_SUCCESS : CL_INVALID_COMMAND_QUEUE;
}

cl_int ValidateReleaseCommandQueue(cl_command_queue command_queue)
{
    return CommandQueue::IsValid(command_queue) ? CL_SUCCESS : CL_INVALID_COMMAND_QUEUE;
}

cl_int ValidateGetCommandQueueInfo(cl_command_queue command_queue,
                                   CommandQueueInfo param_name,
                                   size_t param_value_size,
                                   const void *param_value,
                                   const size_t *param_value_size_ret)
{
    if (!CommandQueue::IsValid(command_queue))
    {
        return CL_INVALID_COMMAND_QUEUE;
    }
    const Device &device = static_cast<const CommandQueue *>(command_queue)->getDevice();
    if (param_name == CommandQueueInfo::InvalidEnum ||
        (param_name == CommandQueueInfo::Size && !device.isVersionOrNewer(2u, 0u)) ||
        (param_name == CommandQueueInfo::DeviceDefault && !device.isVersionOrNewer(2u, 1u)) ||
        (param_name == CommandQueueInfo::PropertiesArray && !device.isVersionOrNewer(3u, 0u)) ||
        (param_value_size == 0u && param_value != nullptr))
    {
        return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

bool ValidateCreateBuffer(cl_context context,
                          cl_mem_flags flags,
                          size_t size,
                          const void *host_ptr,
                          cl_int *errcode_ret)
{
    if (!Context::IsValid(context))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_CONTEXT, false);
    }
    const Context &ctx = *static_cast<Context *>(context);
    if (!ValidateMemoryFlags(flags, ctx.getPlatform()))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
    }
    if (size == 0u)
    {
        ANGLE_ERROR_RETURN(CL_INVALID_BUFFER_SIZE, false);
    }
    for (const DeviceRefPtr &device : ctx.getDevices())
    {
        if (size > device->getInfo().mMaxMemAllocSize)
        {
            ANGLE_ERROR_RETURN(CL_INVALID_BUFFER_SIZE, false);
        }
    }
    if ((host_ptr != nullptr) != ((flags & (CL_MEM_USE_HOST_PTR | CL_MEM_COPY_HOST_PTR)) != 0u))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_HOST_PTR, false);
    }
    return true;
}

cl_int ValidateRetainMemObject(cl_mem memobj)
{
    return Memory::IsValid(memobj) ? CL_SUCCESS : CL_INVALID_MEM_OBJECT;
}

cl_int ValidateReleaseMemObject(cl_mem memobj)
{
    return Memory::IsValid(memobj) ? CL_SUCCESS : CL_INVALID_MEM_OBJECT;
}

cl_int ValidateGetSupportedImageFormats(cl_context context,
                                        cl_mem_flags flags,
                                        MemObjectType image_type,
                                        cl_uint num_entries,
                                        const cl_image_format *image_formats,
                                        const cl_uint *num_image_formats)
{
    return CL_SUCCESS;
}

cl_int ValidateGetMemObjectInfo(cl_mem memobj,
                                MemInfo param_name,
                                size_t param_value_size,
                                const void *param_value,
                                const size_t *param_value_size_ret)
{
    if (!Memory::IsValid(memobj))
    {
        return CL_INVALID_MEM_OBJECT;
    }
    const Platform &platform = static_cast<const Memory *>(memobj)->getContext().getPlatform();
    if (param_name == MemInfo::InvalidEnum ||
        (param_name == MemInfo::AssociatedMemObject && !platform.isVersionOrNewer(1u, 1u)) ||
        (param_name == MemInfo::Offset && !platform.isVersionOrNewer(1u, 1u)) ||
        (param_name == MemInfo::UsesSVM_Pointer && !platform.isVersionOrNewer(2u, 0u)) ||
        (param_name == MemInfo::Properties && !platform.isVersionOrNewer(3u, 0u)) ||
        (param_value_size == 0u && param_value != nullptr))
    {
        return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

cl_int ValidateGetImageInfo(cl_mem image,
                            ImageInfo param_name,
                            size_t param_value_size,
                            const void *param_value,
                            const size_t *param_value_size_ret)
{
    if (!Image::IsValid(image))
    {
        return CL_INVALID_MEM_OBJECT;
    }
    const Platform &platform = static_cast<const Image *>(image)->getContext().getPlatform();
    if (param_name == ImageInfo::InvalidEnum ||
        ((param_name == ImageInfo::ArraySize || param_name == ImageInfo::Buffer ||
          param_name == ImageInfo::NumMipLevels || param_name == ImageInfo::NumSamples) &&
         !platform.isVersionOrNewer(1u, 2u)) ||
        (param_value_size == 0u && param_value != nullptr))
    {
        return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

cl_int ValidateRetainSampler(cl_sampler sampler)
{
    return Sampler::IsValid(sampler) ? CL_SUCCESS : CL_INVALID_SAMPLER;
}

cl_int ValidateReleaseSampler(cl_sampler sampler)
{
    return Sampler::IsValid(sampler) ? CL_SUCCESS : CL_INVALID_SAMPLER;
}

cl_int ValidateGetSamplerInfo(cl_sampler sampler,
                              SamplerInfo param_name,
                              size_t param_value_size,
                              const void *param_value,
                              const size_t *param_value_size_ret)
{
    if (!Sampler::IsValid(sampler))
    {
        return CL_INVALID_SAMPLER;
    }
    const Platform &platform = static_cast<const Sampler *>(sampler)->getContext().getPlatform();
    if (param_name == SamplerInfo::InvalidEnum ||
        (param_name == SamplerInfo::Properties && !platform.isVersionOrNewer(3u, 0u)) ||
        (param_value_size == 0u && param_value != nullptr))
    {
        return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

bool ValidateCreateProgramWithSource(cl_context context,
                                     cl_uint count,
                                     const char **strings,
                                     const size_t *lengths,
                                     cl_int *errcode_ret)
{
    if (!Context::IsValid(context))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_CONTEXT, false);
    }
    if (count == 0u || strings == nullptr)
    {
        ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
    }
    while (count-- != 0u)
    {
        if (*strings++ == nullptr)
        {
            ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
        }
    }
    return true;
}

bool ValidateCreateProgramWithBinary(cl_context context,
                                     cl_uint num_devices,
                                     const cl_device_id *device_list,
                                     const size_t *lengths,
                                     const unsigned char **binaries,
                                     const cl_int *binary_status,
                                     cl_int *errcode_ret)
{
    if (!Context::IsValid(context))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_CONTEXT, false);
    }
    if (num_devices == 0u || device_list == nullptr || lengths == nullptr || binaries == nullptr)
    {
        ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
    }
    const Context &ctx = *static_cast<Context *>(context);
    while (num_devices-- != 0u)
    {
        if (!ctx.hasDevice(*device_list++))
        {
            ANGLE_ERROR_RETURN(CL_INVALID_DEVICE, false);
        }
        if (*lengths++ == 0u || *binaries++ == nullptr)
        {
            ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
        }
    }
    return true;
}

cl_int ValidateRetainProgram(cl_program program)
{
    return Program::IsValid(program) ? CL_SUCCESS : CL_INVALID_PROGRAM;
}

cl_int ValidateReleaseProgram(cl_program program)
{
    return Program::IsValid(program) ? CL_SUCCESS : CL_INVALID_PROGRAM;
}

cl_int ValidateBuildProgram(cl_program program,
                            cl_uint num_devices,
                            const cl_device_id *device_list,
                            const char *options,
                            void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
                            const void *user_data)
{
    return CL_SUCCESS;
}

cl_int ValidateGetProgramInfo(cl_program program,
                              ProgramInfo param_name,
                              size_t param_value_size,
                              const void *param_value,
                              const size_t *param_value_size_ret)
{
    if (!Program::IsValid(program))
    {
        return CL_INVALID_PROGRAM;
    }
    const Platform &platform = static_cast<const Program *>(program)->getContext().getPlatform();
    if (param_name == ProgramInfo::InvalidEnum ||
        ((param_name == ProgramInfo::NumKernels || param_name == ProgramInfo::KernelNames) &&
         !platform.isVersionOrNewer(1u, 2u)) ||
        (param_name == ProgramInfo::IL && !platform.isVersionOrNewer(2u, 1u)) ||
        ((param_name == ProgramInfo::ScopeGlobalCtorsPresent ||
          param_name == ProgramInfo::ScopeGlobalDtorsPresent) &&
         !platform.isVersionOrNewer(2u, 2u)) ||
        (param_value_size == 0u && param_value != nullptr))
    {
        return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

cl_int ValidateGetProgramBuildInfo(cl_program program,
                                   cl_device_id device,
                                   ProgramBuildInfo param_name,
                                   size_t param_value_size,
                                   const void *param_value,
                                   const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

bool ValidateCreateKernel(cl_program program, const char *kernel_name, cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateCreateKernelsInProgram(cl_program program,
                                      cl_uint num_kernels,
                                      const cl_kernel *kernels,
                                      const cl_uint *num_kernels_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateRetainKernel(cl_kernel kernel)
{
    return CL_SUCCESS;
}

cl_int ValidateReleaseKernel(cl_kernel kernel)
{
    return CL_SUCCESS;
}

cl_int ValidateSetKernelArg(cl_kernel kernel,
                            cl_uint arg_index,
                            size_t arg_size,
                            const void *arg_value)
{
    return CL_SUCCESS;
}

cl_int ValidateGetKernelInfo(cl_kernel kernel,
                             KernelInfo param_name,
                             size_t param_value_size,
                             const void *param_value,
                             const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateGetKernelWorkGroupInfo(cl_kernel kernel,
                                      cl_device_id device,
                                      KernelWorkGroupInfo param_name,
                                      size_t param_value_size,
                                      const void *param_value,
                                      const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateWaitForEvents(cl_uint num_events, const cl_event *event_list)
{
    return CL_SUCCESS;
}

cl_int ValidateGetEventInfo(cl_event event,
                            EventInfo param_name,
                            size_t param_value_size,
                            const void *param_value,
                            const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateRetainEvent(cl_event event)
{
    return CL_SUCCESS;
}

cl_int ValidateReleaseEvent(cl_event event)
{
    return CL_SUCCESS;
}

cl_int ValidateGetEventProfilingInfo(cl_event event,
                                     ProfilingInfo param_name,
                                     size_t param_value_size,
                                     const void *param_value,
                                     const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateFlush(cl_command_queue command_queue)
{
    return CL_SUCCESS;
}

cl_int ValidateFinish(cl_command_queue command_queue)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueReadBuffer(cl_command_queue command_queue,
                                 cl_mem buffer,
                                 cl_bool blocking_read,
                                 size_t offset,
                                 size_t size,
                                 const void *ptr,
                                 cl_uint num_events_in_wait_list,
                                 const cl_event *event_wait_list,
                                 const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueWriteBuffer(cl_command_queue command_queue,
                                  cl_mem buffer,
                                  cl_bool blocking_write,
                                  size_t offset,
                                  size_t size,
                                  const void *ptr,
                                  cl_uint num_events_in_wait_list,
                                  const cl_event *event_wait_list,
                                  const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueCopyBuffer(cl_command_queue command_queue,
                                 cl_mem src_buffer,
                                 cl_mem dst_buffer,
                                 size_t src_offset,
                                 size_t dst_offset,
                                 size_t size,
                                 cl_uint num_events_in_wait_list,
                                 const cl_event *event_wait_list,
                                 const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueReadImage(cl_command_queue command_queue,
                                cl_mem image,
                                cl_bool blocking_read,
                                const size_t *origin,
                                const size_t *region,
                                size_t row_pitch,
                                size_t slice_pitch,
                                const void *ptr,
                                cl_uint num_events_in_wait_list,
                                const cl_event *event_wait_list,
                                const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueWriteImage(cl_command_queue command_queue,
                                 cl_mem image,
                                 cl_bool blocking_write,
                                 const size_t *origin,
                                 const size_t *region,
                                 size_t input_row_pitch,
                                 size_t input_slice_pitch,
                                 const void *ptr,
                                 cl_uint num_events_in_wait_list,
                                 const cl_event *event_wait_list,
                                 const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueCopyImage(cl_command_queue command_queue,
                                cl_mem src_image,
                                cl_mem dst_image,
                                const size_t *src_origin,
                                const size_t *dst_origin,
                                const size_t *region,
                                cl_uint num_events_in_wait_list,
                                const cl_event *event_wait_list,
                                const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueCopyImageToBuffer(cl_command_queue command_queue,
                                        cl_mem src_image,
                                        cl_mem dst_buffer,
                                        const size_t *src_origin,
                                        const size_t *region,
                                        size_t dst_offset,
                                        cl_uint num_events_in_wait_list,
                                        const cl_event *event_wait_list,
                                        const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueCopyBufferToImage(cl_command_queue command_queue,
                                        cl_mem src_buffer,
                                        cl_mem dst_image,
                                        size_t src_offset,
                                        const size_t *dst_origin,
                                        const size_t *region,
                                        cl_uint num_events_in_wait_list,
                                        const cl_event *event_wait_list,
                                        const cl_event *event)
{
    return CL_SUCCESS;
}

bool ValidateEnqueueMapBuffer(cl_command_queue command_queue,
                              cl_mem buffer,
                              cl_bool blocking_map,
                              cl_map_flags map_flags,
                              size_t offset,
                              size_t size,
                              cl_uint num_events_in_wait_list,
                              const cl_event *event_wait_list,
                              const cl_event *event,
                              cl_int *errcode_ret)
{
    return true;
}

bool ValidateEnqueueMapImage(cl_command_queue command_queue,
                             cl_mem image,
                             cl_bool blocking_map,
                             cl_map_flags map_flags,
                             const size_t *origin,
                             const size_t *region,
                             const size_t *image_row_pitch,
                             const size_t *image_slice_pitch,
                             cl_uint num_events_in_wait_list,
                             const cl_event *event_wait_list,
                             const cl_event *event,
                             cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateEnqueueUnmapMemObject(cl_command_queue command_queue,
                                     cl_mem memobj,
                                     const void *mapped_ptr,
                                     cl_uint num_events_in_wait_list,
                                     const cl_event *event_wait_list,
                                     const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueNDRangeKernel(cl_command_queue command_queue,
                                    cl_kernel kernel,
                                    cl_uint work_dim,
                                    const size_t *global_work_offset,
                                    const size_t *global_work_size,
                                    const size_t *local_work_size,
                                    cl_uint num_events_in_wait_list,
                                    const cl_event *event_wait_list,
                                    const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueNativeKernel(cl_command_queue command_queue,
                                   void(CL_CALLBACK *user_func)(void *),
                                   const void *args,
                                   size_t cb_args,
                                   cl_uint num_mem_objects,
                                   const cl_mem *mem_list,
                                   const void **args_mem_loc,
                                   cl_uint num_events_in_wait_list,
                                   const cl_event *event_wait_list,
                                   const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateSetCommandQueueProperty(cl_command_queue command_queue,
                                       cl_command_queue_properties properties,
                                       cl_bool enable,
                                       const cl_command_queue_properties *old_properties)
{
    if (!CommandQueue::IsValid(command_queue))
    {
        return CL_INVALID_COMMAND_QUEUE;
    }
    // Fails if properties bitfield is not zero after masking out all allowed values
    if ((properties & ~(CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE)) != 0u)
    {
        return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

bool ValidateCreateImage2D(cl_context context,
                           cl_mem_flags flags,
                           const cl_image_format *image_format,
                           size_t image_width,
                           size_t image_height,
                           size_t image_row_pitch,
                           const void *host_ptr,
                           cl_int *errcode_ret)
{
    const cl_image_desc desc = {CL_MEM_OBJECT_IMAGE2D, image_width, image_height, 0u, 0u,
                                image_row_pitch,       0u,          0u,           0u, {nullptr}};
    return ValidateCreateImage(context, flags, image_format, &desc, host_ptr, errcode_ret);
}

bool ValidateCreateImage3D(cl_context context,
                           cl_mem_flags flags,
                           const cl_image_format *image_format,
                           size_t image_width,
                           size_t image_height,
                           size_t image_depth,
                           size_t image_row_pitch,
                           size_t image_slice_pitch,
                           const void *host_ptr,
                           cl_int *errcode_ret)
{
    const cl_image_desc desc = {
        CL_MEM_OBJECT_IMAGE3D, image_width,       image_height, image_depth, 0u,
        image_row_pitch,       image_slice_pitch, 0u,           0u,          {nullptr}};
    return ValidateCreateImage(context, flags, image_format, &desc, host_ptr, errcode_ret);
}

cl_int ValidateEnqueueMarker(cl_command_queue command_queue, const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueWaitForEvents(cl_command_queue command_queue,
                                    cl_uint num_events,
                                    const cl_event *event_list)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueBarrier(cl_command_queue command_queue)
{
    return CL_SUCCESS;
}

cl_int ValidateUnloadCompiler()
{
    return CL_SUCCESS;
}

bool ValidateGetExtensionFunctionAddress(const char *func_name)
{
    return func_name != nullptr && *func_name != '\0';
}

bool ValidateCreateCommandQueue(cl_context context,
                                cl_device_id device,
                                cl_command_queue_properties properties,
                                cl_int *errcode_ret)
{
    if (!Context::IsValid(context))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_CONTEXT, false);
    }
    if (!static_cast<Context *>(context)->hasDevice(device))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_DEVICE, false);
    }
    // Fails if properties bitfield is not zero after masking out all allowed values
    if ((properties & ~(CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE)) != 0u)
    {
        ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
    }
    return true;
}

bool ValidateCreateSampler(cl_context context,
                           cl_bool normalized_coords,
                           AddressingMode addressing_mode,
                           FilterMode filter_mode,
                           cl_int *errcode_ret)
{
    if (!Context::IsValid(context))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_CONTEXT, false);
    }
    if ((normalized_coords != CL_FALSE && normalized_coords != CL_TRUE) ||
        addressing_mode == AddressingMode::InvalidEnum || filter_mode == FilterMode::InvalidEnum)
    {
        ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
    }
    if (!static_cast<Context *>(context)->supportsImages())
    {
        ANGLE_ERROR_RETURN(CL_INVALID_OPERATION, false);
    }
    return true;
}

cl_int ValidateEnqueueTask(cl_command_queue command_queue,
                           cl_kernel kernel,
                           cl_uint num_events_in_wait_list,
                           const cl_event *event_wait_list,
                           const cl_event *event)
{
    return CL_SUCCESS;
}

// CL 1.1
bool ValidateCreateSubBuffer(cl_mem buffer,
                             cl_mem_flags flags,
                             cl_buffer_create_type buffer_create_type,
                             const void *buffer_create_info,
                             cl_int *errcode_ret)
{
    if (!Buffer::IsValid(buffer))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_MEM_OBJECT, false);
    }
    const Buffer &buf = *static_cast<const Buffer *>(buffer);
    if (buf.isSubBuffer() || !buf.getContext().getPlatform().isVersionOrNewer(1u, 1u))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_MEM_OBJECT, false);
    }
    const cl_mem_flags bufFlags = buf.getFlags();
    if (!ValidateMemoryFlags(flags, buf.getContext().getPlatform()) ||
        ((bufFlags & CL_MEM_WRITE_ONLY) != 0u &&
         (flags & (CL_MEM_READ_WRITE | CL_MEM_READ_ONLY)) != 0u) ||
        ((bufFlags & CL_MEM_READ_ONLY) != 0u &&
         (flags & (CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY)) != 0u) ||
        (flags & (CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR | CL_MEM_COPY_HOST_PTR)) != 0u ||
        ((bufFlags & CL_MEM_HOST_WRITE_ONLY) != 0u && (flags & CL_MEM_HOST_READ_ONLY) != 0u) ||
        ((bufFlags & CL_MEM_HOST_READ_ONLY) != 0u && (flags & CL_MEM_HOST_WRITE_ONLY) != 0u) ||
        ((bufFlags & CL_MEM_HOST_NO_ACCESS) != 0u &&
         (flags & (CL_MEM_HOST_READ_ONLY | CL_MEM_HOST_WRITE_ONLY)) != 0u) ||
        buffer_create_type != CL_BUFFER_CREATE_TYPE_REGION || buffer_create_info == nullptr ||
        !buf.isRegionValid(*static_cast<const cl_buffer_region *>(buffer_create_info)))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
    }
    if (static_cast<const cl_buffer_region *>(buffer_create_info)->size == 0u)
    {
        ANGLE_ERROR_RETURN(CL_INVALID_BUFFER_SIZE, false);
    }
    return true;
}

cl_int ValidateSetMemObjectDestructorCallback(cl_mem memobj,
                                              void(CL_CALLBACK *pfn_notify)(cl_mem memobj,
                                                                            void *user_data),
                                              const void *user_data)
{
    return CL_SUCCESS;
}

bool ValidateCreateUserEvent(cl_context context, cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateSetUserEventStatus(cl_event event, cl_int execution_status)
{
    return CL_SUCCESS;
}

cl_int ValidateSetEventCallback(cl_event event,
                                cl_int command_exec_callback_type,
                                void(CL_CALLBACK *pfn_notify)(cl_event event,
                                                              cl_int event_command_status,
                                                              void *user_data),
                                const void *user_data)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueReadBufferRect(cl_command_queue command_queue,
                                     cl_mem buffer,
                                     cl_bool blocking_read,
                                     const size_t *buffer_origin,
                                     const size_t *host_origin,
                                     const size_t *region,
                                     size_t buffer_row_pitch,
                                     size_t buffer_slice_pitch,
                                     size_t host_row_pitch,
                                     size_t host_slice_pitch,
                                     const void *ptr,
                                     cl_uint num_events_in_wait_list,
                                     const cl_event *event_wait_list,
                                     const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueWriteBufferRect(cl_command_queue command_queue,
                                      cl_mem buffer,
                                      cl_bool blocking_write,
                                      const size_t *buffer_origin,
                                      const size_t *host_origin,
                                      const size_t *region,
                                      size_t buffer_row_pitch,
                                      size_t buffer_slice_pitch,
                                      size_t host_row_pitch,
                                      size_t host_slice_pitch,
                                      const void *ptr,
                                      cl_uint num_events_in_wait_list,
                                      const cl_event *event_wait_list,
                                      const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueCopyBufferRect(cl_command_queue command_queue,
                                     cl_mem src_buffer,
                                     cl_mem dst_buffer,
                                     const size_t *src_origin,
                                     const size_t *dst_origin,
                                     const size_t *region,
                                     size_t src_row_pitch,
                                     size_t src_slice_pitch,
                                     size_t dst_row_pitch,
                                     size_t dst_slice_pitch,
                                     cl_uint num_events_in_wait_list,
                                     const cl_event *event_wait_list,
                                     const cl_event *event)
{
    return CL_SUCCESS;
}

// CL 1.2
cl_int ValidateCreateSubDevices(cl_device_id in_device,
                                const cl_device_partition_property *properties,
                                cl_uint num_devices,
                                const cl_device_id *out_devices,
                                const cl_uint *num_devices_ret)
{
    if (!Device::IsValid(in_device) || !static_cast<Device *>(in_device)->isVersionOrNewer(1u, 2u))
    {
        return CL_INVALID_DEVICE;
    }
    if (properties == nullptr || (*properties != CL_DEVICE_PARTITION_EQUALLY &&
                                  *properties != CL_DEVICE_PARTITION_BY_COUNTS &&
                                  *properties != CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN))
    {
        return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

cl_int ValidateRetainDevice(cl_device_id device)
{
    return Device::IsValid(device) && static_cast<Device *>(device)->isVersionOrNewer(1u, 2u)
               ? CL_SUCCESS
               : CL_INVALID_DEVICE;
}

cl_int ValidateReleaseDevice(cl_device_id device)
{
    return Device::IsValid(device) && static_cast<Device *>(device)->isVersionOrNewer(1u, 2u)
               ? CL_SUCCESS
               : CL_INVALID_DEVICE;
}

bool ValidateCreateImage(cl_context context,
                         cl_mem_flags flags,
                         const cl_image_format *image_format,
                         const cl_image_desc *image_desc,
                         const void *host_ptr,
                         cl_int *errcode_ret)
{
    if (!Context::IsValid(context))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_CONTEXT, false);
    }
    const Context &ctx = *static_cast<Context *>(context);
    if (!ValidateMemoryFlags(flags, ctx.getPlatform()))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
    }
    if (!ValidateImageFormat(image_format, ctx.getPlatform()))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR, false);
    }

    const size_t elemSize = GetElementSize(*image_format);
    if (elemSize == 0u)
    {
        ERR() << "Failed to calculate image element size";
    }
    const size_t rowPitch = image_desc == nullptr ? 0u
                                                  : (image_desc->image_row_pitch != 0u
                                                         ? image_desc->image_row_pitch
                                                         : image_desc->image_width * elemSize);

    const size_t sliceSize = image_desc == nullptr
                                 ? 0u
                                 : rowPitch * (image_desc->image_type == CL_MEM_OBJECT_IMAGE1D_ARRAY
                                                   ? 1u
                                                   : image_desc->image_height);

    if (image_desc == nullptr ||
        (image_desc->image_type != CL_MEM_OBJECT_IMAGE1D &&
         image_desc->image_type != CL_MEM_OBJECT_IMAGE2D &&
         image_desc->image_type != CL_MEM_OBJECT_IMAGE3D &&
         image_desc->image_type != CL_MEM_OBJECT_IMAGE1D_ARRAY &&
         image_desc->image_type != CL_MEM_OBJECT_IMAGE2D_ARRAY &&
         image_desc->image_type != CL_MEM_OBJECT_IMAGE1D_BUFFER) ||
        image_desc->image_width == 0u ||
        (image_desc->image_height == 0u &&
         (image_desc->image_type == CL_MEM_OBJECT_IMAGE2D ||
          image_desc->image_type == CL_MEM_OBJECT_IMAGE3D ||
          image_desc->image_type == CL_MEM_OBJECT_IMAGE2D_ARRAY)) ||
        (image_desc->image_depth == 0u && image_desc->image_type == CL_MEM_OBJECT_IMAGE3D) ||
        (image_desc->image_array_size == 0u &&
         (image_desc->image_type == CL_MEM_OBJECT_IMAGE1D_ARRAY ||
          image_desc->image_type == CL_MEM_OBJECT_IMAGE2D_ARRAY)) ||
        (image_desc->image_row_pitch != 0u &&
         (host_ptr == nullptr || image_desc->image_row_pitch < image_desc->image_width * elemSize ||
          (image_desc->image_row_pitch % elemSize) != 0u)) ||
        (image_desc->image_slice_pitch != 0u &&
         (host_ptr == nullptr || image_desc->image_slice_pitch < sliceSize ||
          (image_desc->image_slice_pitch % rowPitch) != 0u)) ||
        image_desc->num_mip_levels != 0u || image_desc->num_samples != 0u ||
        (image_desc->buffer != nullptr &&
         (!Buffer::IsValid(image_desc->buffer) ||
          (image_desc->image_type != CL_MEM_OBJECT_IMAGE1D_BUFFER &&
           image_desc->image_type != CL_MEM_OBJECT_IMAGE2D)) &&
         (!Image::IsValid(image_desc->buffer) || image_desc->image_type != CL_MEM_OBJECT_IMAGE2D)))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_IMAGE_DESCRIPTOR, false);
    }

    if (!ctx.supportsImages())
    {
        ANGLE_ERROR_RETURN(CL_INVALID_OPERATION, false);
    }

    // Fail if image dimensions exceed supported maximum of all devices
    const DeviceRefList &devices = ctx.getDevices();
    if (std::find_if(devices.cbegin(), devices.cend(), [&](const DeviceRefPtr &ptr) {
            switch (image_desc->image_type)
            {
                case CL_MEM_OBJECT_IMAGE1D:
                    return image_desc->image_width <= ptr->getInfo().mImage2D_MaxWidth;
                case CL_MEM_OBJECT_IMAGE2D:
                    return image_desc->image_width <= ptr->getInfo().mImage2D_MaxWidth &&
                           image_desc->image_height <= ptr->getInfo().mImage2D_MaxHeight;
                case CL_MEM_OBJECT_IMAGE3D:
                    return image_desc->image_width <= ptr->getInfo().mImage3D_MaxWidth &&
                           image_desc->image_height <= ptr->getInfo().mImage3D_MaxHeight &&
                           image_desc->image_depth <= ptr->getInfo().mImage3D_MaxDepth;
                case CL_MEM_OBJECT_IMAGE1D_ARRAY:
                    return image_desc->image_width <= ptr->getInfo().mImage2D_MaxWidth &&
                           image_desc->image_array_size <= ptr->getInfo().mImageMaxArraySize;
                case CL_MEM_OBJECT_IMAGE2D_ARRAY:
                    return image_desc->image_width <= ptr->getInfo().mImage2D_MaxWidth &&
                           image_desc->image_height <= ptr->getInfo().mImage2D_MaxHeight &&
                           image_desc->image_array_size <= ptr->getInfo().mImageMaxArraySize;
                case CL_MEM_OBJECT_IMAGE1D_BUFFER:
                    return image_desc->image_width <= ptr->getInfo().mImageMaxBufferSize;
            }
            return false;
        }) == devices.cend())
    {
        ANGLE_ERROR_RETURN(CL_INVALID_IMAGE_SIZE, false);
    }

    if ((host_ptr != nullptr) != ((flags & (CL_MEM_USE_HOST_PTR | CL_MEM_COPY_HOST_PTR)) != 0u))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_HOST_PTR, false);
    }
    return true;
}

bool ValidateCreateProgramWithBuiltInKernels(cl_context context,
                                             cl_uint num_devices,
                                             const cl_device_id *device_list,
                                             const char *kernel_names,
                                             cl_int *errcode_ret)
{
    if (!Context::IsValid(context))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_CONTEXT, false);
    }
    const Context &ctx = *static_cast<Context *>(context);
    if (!ctx.getPlatform().isVersionOrNewer(1u, 2u))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_CONTEXT, false);
    }
    if (num_devices == 0u || device_list == nullptr || kernel_names == nullptr)
    {
        ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
    }
    for (size_t index = 0u; index < num_devices; ++index)
    {
        if (!ctx.hasDevice(device_list[index]))
        {
            ANGLE_ERROR_RETURN(CL_INVALID_DEVICE, false);
        }
    }
    // Check for support of each kernel name terminated by semi-colon or end of string
    const char *start = kernel_names;
    do
    {
        const char *end = start;
        while (*end != '\0' && *end != ';')
        {
            ++end;
        }
        const size_t length = end - start;
        if (length != 0u && !ctx.supportsBuiltInKernel(std::string(start, length)))
        {
            ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
        }
        start = end;
    } while (*start++ != '\0');
    return true;
}

cl_int ValidateCompileProgram(cl_program program,
                              cl_uint num_devices,
                              const cl_device_id *device_list,
                              const char *options,
                              cl_uint num_input_headers,
                              const cl_program *input_headers,
                              const char **header_include_names,
                              void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
                              const void *user_data)
{
    return CL_SUCCESS;
}

bool ValidateLinkProgram(cl_context context,
                         cl_uint num_devices,
                         const cl_device_id *device_list,
                         const char *options,
                         cl_uint num_input_programs,
                         const cl_program *input_programs,
                         void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
                         const void *user_data,
                         cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateUnloadPlatformCompiler(cl_platform_id platform)
{
    return CL_SUCCESS;
}

cl_int ValidateGetKernelArgInfo(cl_kernel kernel,
                                cl_uint arg_index,
                                KernelArgInfo param_name,
                                size_t param_value_size,
                                const void *param_value,
                                const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueFillBuffer(cl_command_queue command_queue,
                                 cl_mem buffer,
                                 const void *pattern,
                                 size_t pattern_size,
                                 size_t offset,
                                 size_t size,
                                 cl_uint num_events_in_wait_list,
                                 const cl_event *event_wait_list,
                                 const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueFillImage(cl_command_queue command_queue,
                                cl_mem image,
                                const void *fill_color,
                                const size_t *origin,
                                const size_t *region,
                                cl_uint num_events_in_wait_list,
                                const cl_event *event_wait_list,
                                const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueMigrateMemObjects(cl_command_queue command_queue,
                                        cl_uint num_mem_objects,
                                        const cl_mem *mem_objects,
                                        cl_mem_migration_flags flags,
                                        cl_uint num_events_in_wait_list,
                                        const cl_event *event_wait_list,
                                        const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueMarkerWithWaitList(cl_command_queue command_queue,
                                         cl_uint num_events_in_wait_list,
                                         const cl_event *event_wait_list,
                                         const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueBarrierWithWaitList(cl_command_queue command_queue,
                                          cl_uint num_events_in_wait_list,
                                          const cl_event *event_wait_list,
                                          const cl_event *event)
{
    return CL_SUCCESS;
}

bool ValidateGetExtensionFunctionAddressForPlatform(cl_platform_id platform, const char *func_name)
{
    return Platform::IsValid(platform) && func_name != nullptr && *func_name != '\0';
}

// CL 2.0
bool ValidateCreateCommandQueueWithProperties(cl_context context,
                                              cl_device_id device,
                                              const cl_queue_properties *properties,
                                              cl_int *errcode_ret)
{
    if (!Context::IsValid(context))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_CONTEXT, false);
    }
    if (!static_cast<Context *>(context)->hasDevice(device) ||
        !static_cast<Device *>(device)->isVersionOrNewer(2u, 0u))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_DEVICE, false);
    }
    if (properties != nullptr)
    {
        while (*properties != 0)
        {
            switch (*properties++)
            {
                case CL_QUEUE_PROPERTIES:
                {
                    const cl_command_queue_properties props = *properties++;
                    // Fails if properties bitfield is not zero after masking out allowed values
                    if ((props &
                         ~(CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE |
                           CL_QUEUE_ON_DEVICE | CL_QUEUE_ON_DEVICE_DEFAULT)) != 0u ||
                        // Fails for invalid value combinations
                        ((props & CL_QUEUE_ON_DEVICE) != 0u &&
                         (props & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) == 0u) ||
                        ((props & CL_QUEUE_ON_DEVICE_DEFAULT) != 0u &&
                         (props & CL_QUEUE_ON_DEVICE) == 0u))
                    {
                        ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
                    }
                    break;
                }
                case CL_QUEUE_SIZE:
                {
                    cl_uint maxSize = 0u;
                    if (static_cast<Device *>(device)->getInfoUInt(DeviceInfo::QueueOnDeviceMaxSize,
                                                                   &maxSize) != CL_SUCCESS ||
                        *properties > maxSize)
                    {
                        ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
                    }
                    ++properties;
                    break;
                }
                default:
                    ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
            }
        }
    }
    return true;
}

bool ValidateCreatePipe(cl_context context,
                        cl_mem_flags flags,
                        cl_uint pipe_packet_size,
                        cl_uint pipe_max_packets,
                        const cl_pipe_properties *properties,
                        cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateGetPipeInfo(cl_mem pipe,
                           PipeInfo param_name,
                           size_t param_value_size,
                           const void *param_value,
                           const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

bool ValidateSVMAlloc(cl_context context, cl_svm_mem_flags flags, size_t size, cl_uint alignment)
{
    return true;
}

bool ValidateSVMFree(cl_context context, const void *svm_pointer)
{
    return true;
}

bool ValidateCreateSamplerWithProperties(cl_context context,
                                         const cl_sampler_properties *sampler_properties,
                                         cl_int *errcode_ret)
{
    if (!Context::IsValid(context))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_CONTEXT, false);
    }
    if (sampler_properties != nullptr)
    {
        bool hasNormalizedCoords            = false;
        bool hasAddressingMode              = false;
        bool hasFilterMode                  = false;
        const cl_sampler_properties *propIt = sampler_properties;
        while (*propIt != 0)
        {
            switch (*propIt++)
            {
                case CL_SAMPLER_NORMALIZED_COORDS:
                    if (hasNormalizedCoords || (*propIt != CL_FALSE && *propIt != CL_TRUE))
                    {
                        ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
                    }
                    hasNormalizedCoords = true;
                    ++propIt;
                    break;
                case CL_SAMPLER_ADDRESSING_MODE:
                    if (hasAddressingMode || FromCLenum<AddressingMode>(static_cast<CLenum>(
                                                 *propIt++)) == AddressingMode::InvalidEnum)
                    {
                        ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
                    }
                    hasAddressingMode = true;
                    break;
                case CL_SAMPLER_FILTER_MODE:
                    if (hasFilterMode || FromCLenum<FilterMode>(static_cast<CLenum>(*propIt++)) ==
                                             FilterMode::InvalidEnum)
                    {
                        ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
                    }
                    hasFilterMode = true;
                    break;
                default:
                    ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
            }
        }
    }
    if (!static_cast<Context *>(context)->supportsImages())
    {
        ANGLE_ERROR_RETURN(CL_INVALID_OPERATION, false);
    }
    return true;
}

cl_int ValidateSetKernelArgSVMPointer(cl_kernel kernel, cl_uint arg_index, const void *arg_value)
{
    return CL_SUCCESS;
}

cl_int ValidateSetKernelExecInfo(cl_kernel kernel,
                                 KernelExecInfo param_name,
                                 size_t param_value_size,
                                 const void *param_value)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueSVMFree(cl_command_queue command_queue,
                              cl_uint num_svm_pointers,
                              void *const svm_pointers[],
                              void(CL_CALLBACK *pfn_free_func)(cl_command_queue queue,
                                                               cl_uint num_svm_pointers,
                                                               void *svm_pointers[],
                                                               void *user_data),
                              const void *user_data,
                              cl_uint num_events_in_wait_list,
                              const cl_event *event_wait_list,
                              const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueSVMMemcpy(cl_command_queue command_queue,
                                cl_bool blocking_copy,
                                const void *dst_ptr,
                                const void *src_ptr,
                                size_t size,
                                cl_uint num_events_in_wait_list,
                                const cl_event *event_wait_list,
                                const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueSVMMemFill(cl_command_queue command_queue,
                                 const void *svm_ptr,
                                 const void *pattern,
                                 size_t pattern_size,
                                 size_t size,
                                 cl_uint num_events_in_wait_list,
                                 const cl_event *event_wait_list,
                                 const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueSVMMap(cl_command_queue command_queue,
                             cl_bool blocking_map,
                             cl_map_flags flags,
                             const void *svm_ptr,
                             size_t size,
                             cl_uint num_events_in_wait_list,
                             const cl_event *event_wait_list,
                             const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueSVMUnmap(cl_command_queue command_queue,
                               const void *svm_ptr,
                               cl_uint num_events_in_wait_list,
                               const cl_event *event_wait_list,
                               const cl_event *event)
{
    return CL_SUCCESS;
}

// CL 2.1
cl_int ValidateSetDefaultDeviceCommandQueue(cl_context context,
                                            cl_device_id device,
                                            cl_command_queue command_queue)
{
    return CL_SUCCESS;
}

cl_int ValidateGetDeviceAndHostTimer(cl_device_id device,
                                     const cl_ulong *device_timestamp,
                                     const cl_ulong *host_timestamp)
{
    return CL_SUCCESS;
}

cl_int ValidateGetHostTimer(cl_device_id device, const cl_ulong *host_timestamp)
{
    return CL_SUCCESS;
}

bool ValidateCreateProgramWithIL(cl_context context,
                                 const void *il,
                                 size_t length,
                                 cl_int *errcode_ret)
{
    if (!Context::IsValid(context))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_CONTEXT, false);
    }
    const Context &ctx = *static_cast<Context *>(context);
    if (!ctx.getPlatform().isVersionOrNewer(2u, 1u))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_CONTEXT, false);
    }
    if (!ctx.supportsIL())
    {
        ANGLE_ERROR_RETURN(CL_INVALID_OPERATION, false);
    }
    if (il == nullptr || length == 0u)
    {
        ANGLE_ERROR_RETURN(CL_INVALID_VALUE, false);
    }
    return true;
}

bool ValidateCloneKernel(cl_kernel source_kernel, cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateGetKernelSubGroupInfo(cl_kernel kernel,
                                     cl_device_id device,
                                     KernelSubGroupInfo param_name,
                                     size_t input_value_size,
                                     const void *input_value,
                                     size_t param_value_size,
                                     const void *param_value,
                                     const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueSVMMigrateMem(cl_command_queue command_queue,
                                    cl_uint num_svm_pointers,
                                    const void **svm_pointers,
                                    const size_t *sizes,
                                    cl_mem_migration_flags flags,
                                    cl_uint num_events_in_wait_list,
                                    const cl_event *event_wait_list,
                                    const cl_event *event)
{
    return CL_SUCCESS;
}

// CL 2.2
cl_int ValidateSetProgramReleaseCallback(cl_program program,
                                         void(CL_CALLBACK *pfn_notify)(cl_program program,
                                                                       void *user_data),
                                         const void *user_data)
{
    return CL_SUCCESS;
}

cl_int ValidateSetProgramSpecializationConstant(cl_program program,
                                                cl_uint spec_id,
                                                size_t spec_size,
                                                const void *spec_value)
{
    return CL_SUCCESS;
}

// CL 3.0
cl_int ValidateSetContextDestructorCallback(cl_context context,
                                            void(CL_CALLBACK *pfn_notify)(cl_context context,
                                                                          void *user_data),
                                            const void *user_data)
{
    return CL_SUCCESS;
}

bool ValidateCreateBufferWithProperties(cl_context context,
                                        const cl_mem_properties *properties,
                                        cl_mem_flags flags,
                                        size_t size,
                                        const void *host_ptr,
                                        cl_int *errcode_ret)
{
    if (!ValidateCreateBuffer(context, flags, size, host_ptr, errcode_ret))
    {
        return false;
    }
    if (!static_cast<Context *>(context)->getPlatform().isVersionOrNewer(3u, 0u))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_CONTEXT, false);
    }
    if (!ValidateMemoryProperties(properties))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_PROPERTY, false);
    }
    return true;
}

bool ValidateCreateImageWithProperties(cl_context context,
                                       const cl_mem_properties *properties,
                                       cl_mem_flags flags,
                                       const cl_image_format *image_format,
                                       const cl_image_desc *image_desc,
                                       const void *host_ptr,
                                       cl_int *errcode_ret)
{
    if (!ValidateCreateImage(context, flags, image_format, image_desc, host_ptr, errcode_ret))
    {
        return false;
    }
    if (!static_cast<Context *>(context)->getPlatform().isVersionOrNewer(3u, 0u))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_CONTEXT, false);
    }
    if (!ValidateMemoryProperties(properties))
    {
        ANGLE_ERROR_RETURN(CL_INVALID_PROPERTY, false);
    }
    return true;
}

// cl_khr_icd
cl_int ValidateIcdGetPlatformIDsKHR(cl_uint num_entries,
                                    const cl_platform_id *platforms,
                                    const cl_uint *num_platforms)
{
    if ((num_entries == 0u && platforms != nullptr) ||
        (platforms == nullptr && num_platforms == nullptr))
    {
        return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

}  // namespace cl
