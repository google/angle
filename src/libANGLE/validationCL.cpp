//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// validationCL.cpp: Validation functions for generic CL entry point parameters
// based on the OpenCL Specification V3.0.7, see https://www.khronos.org/registry/OpenCL/
// Each used CL error code is preceeded by a citation of the relevant rule in the spec.

#include "libANGLE/validationCL_autogen.h"

#include "libANGLE/cl_utils.h"

#define ANGLE_VALIDATE_VERSION(major, minor)                   \
    do                                                         \
    {                                                          \
        if (version < CL_MAKE_VERSION(major##u, minor##u, 0u)) \
        {                                                      \
            return CL_INVALID_VALUE;                           \
        }                                                      \
    } while (0)

namespace cl
{

namespace
{

cl_int ValidateContextProperties(const cl_context_properties *properties, const Platform *&platform)
{
    platform         = nullptr;
    bool hasUserSync = false;
    if (properties != nullptr)
    {
        while (*properties != 0)
        {
            switch (*properties++)
            {
                case CL_CONTEXT_PLATFORM:
                {
                    // CL_INVALID_PROPERTY if the same property name is specified more than once.
                    if (platform != nullptr)
                    {
                        return CL_INVALID_PROPERTY;
                    }
                    cl_platform_id nativePlatform = reinterpret_cast<cl_platform_id>(*properties++);
                    // CL_INVALID_PLATFORM if platform value specified in properties
                    // is not a valid platform.
                    if (!Platform::IsValid(nativePlatform))
                    {
                        return CL_INVALID_PLATFORM;
                    }
                    platform = &nativePlatform->cast<Platform>();
                    break;
                }
                case CL_CONTEXT_INTEROP_USER_SYNC:
                {
                    // CL_INVALID_PROPERTY if the value specified for a supported property name
                    // is not valid, or if the same property name is specified more than once.
                    if ((*properties != CL_FALSE && *properties != CL_TRUE) || hasUserSync)
                    {
                        return CL_INVALID_PROPERTY;
                    }
                    ++properties;
                    hasUserSync = true;
                    break;
                }
                default:
                {
                    // CL_INVALID_PROPERTY if context property name in properties
                    // is not a supported property name.
                    return CL_INVALID_PROPERTY;
                }
            }
        }
    }
    if (platform == nullptr)
    {
        platform = Platform::GetDefault();
        // CL_INVALID_PLATFORM if properties is NULL and no platform could be selected.
        if (platform == nullptr)
        {
            return CL_INVALID_PLATFORM;
        }
    }
    return CL_SUCCESS;
}

bool ValidateMemoryFlags(MemFlags flags, const Platform &platform)
{
    // CL_MEM_READ_WRITE, CL_MEM_WRITE_ONLY, and CL_MEM_READ_ONLY are mutually exclusive.
    MemFlags allowedFlags(CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY | CL_MEM_READ_ONLY);
    if (!flags.areMutuallyExclusive(CL_MEM_READ_WRITE, CL_MEM_WRITE_ONLY, CL_MEM_READ_ONLY))
    {
        return false;
    }
    // CL_MEM_USE_HOST_PTR is mutually exclusive with either of the other two flags.
    allowedFlags.set(CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR | CL_MEM_COPY_HOST_PTR);
    if (!flags.areMutuallyExclusive(CL_MEM_USE_HOST_PTR,
                                    CL_MEM_ALLOC_HOST_PTR | CL_MEM_COPY_HOST_PTR))
    {
        return false;
    }
    if (platform.isVersionOrNewer(1u, 2u))
    {
        // CL_MEM_HOST_WRITE_ONLY, CL_MEM_HOST_READ_ONLY,
        // and CL_MEM_HOST_NO_ACCESS are mutually exclusive.
        allowedFlags.set(CL_MEM_HOST_WRITE_ONLY | CL_MEM_HOST_READ_ONLY | CL_MEM_HOST_NO_ACCESS);
        if (flags.areMutuallyExclusive(CL_MEM_HOST_WRITE_ONLY, CL_MEM_HOST_READ_ONLY,
                                       CL_MEM_HOST_NO_ACCESS))
        {
            return false;
        }
    }
    if (platform.isVersionOrNewer(2u, 0u))
    {
        allowedFlags.set(CL_MEM_KERNEL_READ_AND_WRITE);
    }
    if (flags.hasOtherBitsThan(allowedFlags))
    {
        return false;
    }
    return true;
}

bool ValidateMemoryProperties(const cl_mem_properties *properties)
{
    if (properties != nullptr)
    {
        // OpenCL 3.0 does not define any optional properties.
        // This function is reserved for extensions and future use.
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
    // CL_INVALID_VALUE if num_entries is equal to zero and platforms is not NULL
    // or if both num_platforms and platforms are NULL.
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
    // CL_INVALID_PLATFORM if platform is not a valid platform.
    if (!Platform::IsValidOrDefault(platform))
    {
        return CL_INVALID_PLATFORM;
    }

    // CL_INVALID_VALUE if param_name is not one of the supported values.
    const cl_version version = platform->cast<Platform>().getVersion();
    switch (param_name)
    {
        case PlatformInfo::HostTimerResolution:
            ANGLE_VALIDATE_VERSION(2, 1);
            break;
        case PlatformInfo::NumericVersion:
        case PlatformInfo::ExtensionsWithVersion:
            ANGLE_VALIDATE_VERSION(3, 0);
            break;
        case PlatformInfo::InvalidEnum:
            return CL_INVALID_VALUE;
        default:
            break;
    }

    return CL_SUCCESS;
}

cl_int ValidateGetDeviceIDs(cl_platform_id platform,
                            DeviceType device_type,
                            cl_uint num_entries,
                            const cl_device_id *devices,
                            const cl_uint *num_devices)
{
    // CL_INVALID_PLATFORM if platform is not a valid platform.
    if (!Platform::IsValidOrDefault(platform))
    {
        return CL_INVALID_PLATFORM;
    }

    // CL_INVALID_DEVICE_TYPE if device_type is not a valid value.
    if (!Device::IsValidType(device_type))
    {
        return CL_INVALID_DEVICE_TYPE;
    }

    // CL_INVALID_VALUE if num_entries is equal to zero and devices is not NULL
    // or if both num_devices and devices are NULL.
    if ((num_entries == 0u && devices != nullptr) || (num_devices == nullptr && devices == nullptr))
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
    // CL_INVALID_DEVICE if device is not a valid device.
    if (!Device::IsValid(device))
    {
        return CL_INVALID_DEVICE;
    }

    // CL_INVALID_VALUE if param_name is not one of the supported values
    // or if param_name is a value that is available as an extension
    // and the corresponding extension is not supported by the device.
    const cl_version version = device->cast<Device>().getVersion();
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
            ANGLE_VALIDATE_VERSION(1, 1);
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
            ANGLE_VALIDATE_VERSION(1, 2);
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
            ANGLE_VALIDATE_VERSION(2, 0);
            break;

        case DeviceInfo::IL_Version:
        case DeviceInfo::MaxNumSubGroups:
        case DeviceInfo::SubGroupIndependentForwardProgress:
            ANGLE_VALIDATE_VERSION(2, 1);
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
            ANGLE_VALIDATE_VERSION(3, 0);
            break;

        case DeviceInfo::InvalidEnum:
            return CL_INVALID_VALUE;
        default:
            break;
    }

    return CL_SUCCESS;
}

cl_int ValidateCreateContext(const cl_context_properties *properties,
                             cl_uint num_devices,
                             const cl_device_id *devices,
                             void(CL_CALLBACK *pfn_notify)(const char *errinfo,
                                                           const void *private_info,
                                                           size_t cb,
                                                           void *user_data),
                             const void *user_data)
{
    const Platform *platform = nullptr;
    const cl_int errorCode   = ValidateContextProperties(properties, platform);
    if (errorCode != CL_SUCCESS)
    {
        return errorCode;
    }

    // CL_INVALID_VALUE if devices is NULL or if num_devices is equal to zero
    // or if pfn_notify is NULL but user_data is not NULL.
    if (devices == nullptr || num_devices == 0u || (pfn_notify == nullptr && user_data != nullptr))
    {
        return CL_INVALID_VALUE;
    }

    // CL_INVALID_DEVICE if any device in devices is not a valid device.
    while (num_devices-- > 0u)
    {
        if (!Device::IsValid(*devices) || &(*devices)->cast<Device>().getPlatform() != platform)
        {
            return CL_INVALID_DEVICE;
        }
        ++devices;
    }

    return CL_SUCCESS;
}

cl_int ValidateCreateContextFromType(const cl_context_properties *properties,
                                     DeviceType device_type,
                                     void(CL_CALLBACK *pfn_notify)(const char *errinfo,
                                                                   const void *private_info,
                                                                   size_t cb,
                                                                   void *user_data),
                                     const void *user_data)
{
    const Platform *platform = nullptr;
    const cl_int errorCode   = ValidateContextProperties(properties, platform);
    if (errorCode != CL_SUCCESS)
    {
        return errorCode;
    }

    // CL_INVALID_DEVICE_TYPE if device_type is not a valid value.
    if (!Device::IsValidType(device_type))
    {
        return CL_INVALID_DEVICE_TYPE;
    }

    // CL_INVALID_VALUE if pfn_notify is NULL but user_data is not NULL.
    if (pfn_notify == nullptr && user_data != nullptr)
    {
        return CL_INVALID_VALUE;
    }

    return CL_SUCCESS;
}

cl_int ValidateRetainContext(cl_context context)
{
    // CL_INVALID_CONTEXT if context is not a valid OpenCL context.
    return Context::IsValid(context) ? CL_SUCCESS : CL_INVALID_CONTEXT;
}

cl_int ValidateReleaseContext(cl_context context)
{
    // CL_INVALID_CONTEXT if context is not a valid OpenCL context.
    return Context::IsValid(context) ? CL_SUCCESS : CL_INVALID_CONTEXT;
}

cl_int ValidateGetContextInfo(cl_context context,
                              ContextInfo param_name,
                              size_t param_value_size,
                              const void *param_value,
                              const size_t *param_value_size_ret)
{
    // CL_INVALID_CONTEXT if context is not a valid context.
    if (!Context::IsValid(context))
    {
        return CL_INVALID_CONTEXT;
    }

    // CL_INVALID_VALUE if param_name is not one of the supported values.
    if (param_name == ContextInfo::InvalidEnum)
    {
        return CL_INVALID_VALUE;
    }

    return CL_SUCCESS;
}

cl_int ValidateRetainCommandQueue(cl_command_queue command_queue)
{
    // CL_INVALID_COMMAND_QUEUE if command_queue is not a valid command-queue.
    return CommandQueue::IsValid(command_queue) ? CL_SUCCESS : CL_INVALID_COMMAND_QUEUE;
}

cl_int ValidateReleaseCommandQueue(cl_command_queue command_queue)
{
    // CL_INVALID_COMMAND_QUEUE if command_queue is not a valid command-queue.
    return CommandQueue::IsValid(command_queue) ? CL_SUCCESS : CL_INVALID_COMMAND_QUEUE;
}

cl_int ValidateGetCommandQueueInfo(cl_command_queue command_queue,
                                   CommandQueueInfo param_name,
                                   size_t param_value_size,
                                   const void *param_value,
                                   const size_t *param_value_size_ret)
{
    // CL_INVALID_COMMAND_QUEUE if command_queue is not a valid command-queue ...
    if (!CommandQueue::IsValid(command_queue))
    {
        return CL_INVALID_COMMAND_QUEUE;
    }
    const CommandQueue &queue = command_queue->cast<CommandQueue>();
    // or if command_queue is not a valid command-queue for param_name.
    if (param_name == CommandQueueInfo::Size && queue.getProperties().isNotSet(CL_QUEUE_ON_DEVICE))
    {
        return CL_INVALID_COMMAND_QUEUE;
    }

    // CL_INVALID_VALUE if param_name is not one of the supported values.
    const cl_version version = queue.getDevice().getVersion();
    switch (param_name)
    {
        case CommandQueueInfo::Size:
            ANGLE_VALIDATE_VERSION(2, 0);
            break;
        case CommandQueueInfo::DeviceDefault:
            ANGLE_VALIDATE_VERSION(2, 1);
            break;
        case CommandQueueInfo::PropertiesArray:
            ANGLE_VALIDATE_VERSION(3, 0);
            break;
        case CommandQueueInfo::InvalidEnum:
            return CL_INVALID_VALUE;
        default:
            break;
    }

    return CL_SUCCESS;
}

cl_int ValidateCreateBuffer(cl_context context, MemFlags flags, size_t size, const void *host_ptr)
{
    // CL_INVALID_CONTEXT if context is not a valid context.
    if (!Context::IsValid(context))
    {
        return CL_INVALID_CONTEXT;
    }
    const Context &ctx = context->cast<Context>();

    // CL_INVALID_VALUE if values specified in flags are not valid
    // as defined in the Memory Flags table.
    if (!ValidateMemoryFlags(flags, ctx.getPlatform()))
    {
        return CL_INVALID_VALUE;
    }

    // CL_INVALID_BUFFER_SIZE if size is 0 ...
    if (size == 0u)
    {
        CL_INVALID_BUFFER_SIZE;
    }
    for (const DevicePtr &device : ctx.getDevices())
    {
        // or if size is greater than CL_DEVICE_MAX_MEM_ALLOC_SIZE for all devices in context.
        if (size > device->getInfo().mMaxMemAllocSize)
        {
            return CL_INVALID_BUFFER_SIZE;
        }
    }

    // CL_INVALID_HOST_PTR
    // if host_ptr is NULL and CL_MEM_USE_HOST_PTR or CL_MEM_COPY_HOST_PTR are set in flags or
    // if host_ptr is not NULL but CL_MEM_COPY_HOST_PTR or CL_MEM_USE_HOST_PTR are not set in flags.
    if ((host_ptr != nullptr) != flags.isSet(CL_MEM_USE_HOST_PTR | CL_MEM_COPY_HOST_PTR))
    {
        return CL_INVALID_HOST_PTR;
    }

    return CL_SUCCESS;
}

cl_int ValidateRetainMemObject(cl_mem memobj)
{
    // CL_INVALID_MEM_OBJECT if memobj is not a valid memory object.
    return Memory::IsValid(memobj) ? CL_SUCCESS : CL_INVALID_MEM_OBJECT;
}

cl_int ValidateReleaseMemObject(cl_mem memobj)
{
    // CL_INVALID_MEM_OBJECT if memobj is not a valid memory object.
    return Memory::IsValid(memobj) ? CL_SUCCESS : CL_INVALID_MEM_OBJECT;
}

cl_int ValidateGetSupportedImageFormats(cl_context context,
                                        MemFlags flags,
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
    // CL_INVALID_MEM_OBJECT if memobj is a not a valid memory object.
    if (!Memory::IsValid(memobj))
    {
        return CL_INVALID_MEM_OBJECT;
    }

    // CL_INVALID_VALUE if param_name is not valid.
    const cl_version version = memobj->cast<Memory>().getContext().getPlatform().getVersion();
    switch (param_name)
    {
        case MemInfo::AssociatedMemObject:
        case MemInfo::Offset:
            ANGLE_VALIDATE_VERSION(1, 1);
            break;
        case MemInfo::UsesSVM_Pointer:
            ANGLE_VALIDATE_VERSION(2, 0);
            break;
        case MemInfo::Properties:
            ANGLE_VALIDATE_VERSION(3, 0);
            break;
        case MemInfo::InvalidEnum:
            return CL_INVALID_VALUE;
        default:
            break;
    }

    return CL_SUCCESS;
}

cl_int ValidateGetImageInfo(cl_mem image,
                            ImageInfo param_name,
                            size_t param_value_size,
                            const void *param_value,
                            const size_t *param_value_size_ret)
{
    // CL_INVALID_MEM_OBJECT if image is a not a valid image object.
    if (!Image::IsValid(image))
    {
        return CL_INVALID_MEM_OBJECT;
    }

    // CL_INVALID_VALUE if param_name is not valid.
    const cl_version version = image->cast<Image>().getContext().getPlatform().getVersion();
    switch (param_name)
    {
        case ImageInfo::ArraySize:
        case ImageInfo::Buffer:
        case ImageInfo::NumMipLevels:
        case ImageInfo::NumSamples:
            ANGLE_VALIDATE_VERSION(1, 2);
            break;
        case ImageInfo::InvalidEnum:
            return CL_INVALID_VALUE;
        default:
            break;
    }

    return CL_SUCCESS;
}

cl_int ValidateRetainSampler(cl_sampler sampler)
{
    // CL_INVALID_SAMPLER if sampler is not a valid sampler object.
    return Sampler::IsValid(sampler) ? CL_SUCCESS : CL_INVALID_SAMPLER;
}

cl_int ValidateReleaseSampler(cl_sampler sampler)
{
    // CL_INVALID_SAMPLER if sampler is not a valid sampler object.
    return Sampler::IsValid(sampler) ? CL_SUCCESS : CL_INVALID_SAMPLER;
}

cl_int ValidateGetSamplerInfo(cl_sampler sampler,
                              SamplerInfo param_name,
                              size_t param_value_size,
                              const void *param_value,
                              const size_t *param_value_size_ret)
{
    // CL_INVALID_SAMPLER if sampler is a not a valid sampler object.
    if (!Sampler::IsValid(sampler))
    {
        return CL_INVALID_SAMPLER;
    }

    // CL_INVALID_VALUE if param_name is not valid.
    const cl_version version = sampler->cast<Sampler>().getContext().getPlatform().getVersion();
    switch (param_name)
    {
        case SamplerInfo::Properties:
            ANGLE_VALIDATE_VERSION(3, 0);
            break;
        case SamplerInfo::InvalidEnum:
            return CL_INVALID_VALUE;
        default:
            break;
    }

    return CL_SUCCESS;
}

cl_int ValidateCreateProgramWithSource(cl_context context,
                                       cl_uint count,
                                       const char **strings,
                                       const size_t *lengths)
{
    // CL_INVALID_CONTEXT if context is not a valid context.
    if (!Context::IsValid(context))
    {
        return CL_INVALID_CONTEXT;
    }

    // CL_INVALID_VALUE if count is zero or if strings or any entry in strings is NULL.
    if (count == 0u || strings == nullptr)
    {
        return CL_INVALID_VALUE;
    }
    while (count-- != 0u)
    {
        if (*strings++ == nullptr)
        {
            return CL_INVALID_VALUE;
        }
    }

    return CL_SUCCESS;
}

cl_int ValidateCreateProgramWithBinary(cl_context context,
                                       cl_uint num_devices,
                                       const cl_device_id *device_list,
                                       const size_t *lengths,
                                       const unsigned char **binaries,
                                       const cl_int *binary_status)
{
    // CL_INVALID_CONTEXT if context is not a valid context.
    if (!Context::IsValid(context))
    {
        return CL_INVALID_CONTEXT;
    }
    const Context &ctx = context->cast<Context>();

    // CL_INVALID_VALUE if device_list is NULL or num_devices is zero.
    // CL_INVALID_VALUE if lengths or binaries is NULL.
    if (device_list == nullptr || num_devices == 0u || lengths == nullptr || binaries == nullptr)
    {
        return CL_INVALID_VALUE;
    }
    while (num_devices-- != 0u)
    {
        // CL_INVALID_DEVICE if any device in device_list
        // is not in the list of devices associated with context.
        if (!ctx.hasDevice(*device_list++))
        {
            return CL_INVALID_DEVICE;
        }

        // CL_INVALID_VALUE if any entry in lengths[i] is zero or binaries[i] is NULL.
        if (*lengths++ == 0u || *binaries++ == nullptr)
        {
            return CL_INVALID_VALUE;
        }
    }

    return CL_SUCCESS;
}

cl_int ValidateRetainProgram(cl_program program)
{
    // CL_INVALID_PROGRAM if program is not a valid program object.
    return Program::IsValid(program) ? CL_SUCCESS : CL_INVALID_PROGRAM;
}

cl_int ValidateReleaseProgram(cl_program program)
{
    // CL_INVALID_PROGRAM if program is not a valid program object.
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
    // CL_INVALID_PROGRAM if program is a not a valid program object.
    if (!Program::IsValid(program))
    {
        return CL_INVALID_PROGRAM;
    }

    // CL_INVALID_VALUE if param_name is not valid.
    const cl_version version = program->cast<Program>().getContext().getPlatform().getVersion();
    switch (param_name)
    {
        case ProgramInfo::NumKernels:
        case ProgramInfo::KernelNames:
            ANGLE_VALIDATE_VERSION(1, 2);
            break;
        case ProgramInfo::IL:
            ANGLE_VALIDATE_VERSION(2, 1);
            break;
        case ProgramInfo::ScopeGlobalCtorsPresent:
        case ProgramInfo::ScopeGlobalDtorsPresent:
            ANGLE_VALIDATE_VERSION(2, 2);
            break;
        case ProgramInfo::InvalidEnum:
            return CL_INVALID_VALUE;
        default:
            break;
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

cl_int ValidateCreateKernel(cl_program program, const char *kernel_name)
{
    // CL_INVALID_PROGRAM if program is not a valid program object.
    if (!Program::IsValid(program))
    {
        return CL_INVALID_PROGRAM;
    }

    // CL_INVALID_VALUE if kernel_name is NULL.
    if (kernel_name == nullptr)
    {
        return CL_INVALID_VALUE;
    }

    return CL_SUCCESS;
}

cl_int ValidateCreateKernelsInProgram(cl_program program,
                                      cl_uint num_kernels,
                                      const cl_kernel *kernels,
                                      const cl_uint *num_kernels_ret)
{
    // CL_INVALID_PROGRAM if program is not a valid program object.
    if (!Program::IsValid(program))
    {
        return CL_INVALID_PROGRAM;
    }

    return CL_SUCCESS;
}

cl_int ValidateRetainKernel(cl_kernel kernel)
{
    // CL_INVALID_KERNEL if kernel is not a valid kernel object.
    return Kernel::IsValid(kernel) ? CL_SUCCESS : CL_INVALID_KERNEL;
}

cl_int ValidateReleaseKernel(cl_kernel kernel)
{
    // CL_INVALID_KERNEL if kernel is not a valid kernel object.
    return Kernel::IsValid(kernel) ? CL_SUCCESS : CL_INVALID_KERNEL;
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
    // CL_INVALID_KERNEL if kernel is a not a valid kernel object.
    if (!Kernel::IsValid(kernel))
    {
        return CL_INVALID_KERNEL;
    }

    // CL_INVALID_VALUE if param_name is not valid.
    const cl_version version =
        kernel->cast<Kernel>().getProgram().getContext().getPlatform().getVersion();
    switch (param_name)
    {
        case KernelInfo::Attributes:
            ANGLE_VALIDATE_VERSION(1, 2);
            break;
        case KernelInfo::InvalidEnum:
            return CL_INVALID_VALUE;
        default:
            break;
    }

    return CL_SUCCESS;
}

cl_int ValidateGetKernelWorkGroupInfo(cl_kernel kernel,
                                      cl_device_id device,
                                      KernelWorkGroupInfo param_name,
                                      size_t param_value_size,
                                      const void *param_value,
                                      const size_t *param_value_size_ret)
{
    // CL_INVALID_KERNEL if kernel is a not a valid kernel object.
    if (!Kernel::IsValid(kernel))
    {
        return CL_INVALID_KERNEL;
    }
    const Kernel &krnl = kernel->cast<Kernel>();

    const Device *dev = nullptr;
    if (device != nullptr)
    {
        // CL_INVALID_DEVICE if device is not in the list of devices associated with kernel ...
        if (krnl.getProgram().getContext().hasDevice(device))
        {
            dev = &device->cast<Device>();
        }
        else
        {
            return CL_INVALID_DEVICE;
        }
    }
    else
    {
        // or if device is NULL but there is more than one device associated with kernel.
        if (krnl.getProgram().getContext().getDevices().size() == 1u)
        {
            dev = krnl.getProgram().getContext().getDevices().front().get();
        }
        else
        {
            return CL_INVALID_DEVICE;
        }
    }

    // CL_INVALID_VALUE if param_name is not valid.
    const cl_version version = krnl.getProgram().getContext().getPlatform().getInfo().mVersion;
    switch (param_name)
    {
        case KernelWorkGroupInfo::GlobalWorkSize:
            ANGLE_VALIDATE_VERSION(1, 2);
            // CL_INVALID_VALUE if param_name is CL_KERNEL_GLOBAL_WORK_SIZE and
            // device is not a custom device and kernel is not a built-in kernel.
            if (!dev->supportsBuiltInKernel(krnl.getInfo().mFunctionName))
            {
                return CL_INVALID_VALUE;
            }
            break;
        case KernelWorkGroupInfo::InvalidEnum:
            return CL_INVALID_VALUE;
        default:
            break;
    }

    return CL_SUCCESS;
}

cl_int ValidateWaitForEvents(cl_uint num_events, const cl_event *event_list)
{
    // CL_INVALID_VALUE if num_events is zero or event_list is NULL.
    if (num_events == 0u || event_list == nullptr)
    {
        return CL_INVALID_VALUE;
    }

    const Context *context = nullptr;
    while (num_events-- != 0u)
    {
        // CL_INVALID_EVENT if event objects specified in event_list are not valid event objects.
        if (!Event::IsValid(*event_list))
        {
            return CL_INVALID_EVENT;
        }

        // CL_INVALID_CONTEXT if events specified in event_list do not belong to the same context.
        const Context *eventContext = &(*event_list++)->cast<Event>().getContext();
        if (context == nullptr)
        {
            context = eventContext;
        }
        else if (context != eventContext)
        {
            return CL_INVALID_CONTEXT;
        }
    }

    return CL_SUCCESS;
}

cl_int ValidateGetEventInfo(cl_event event,
                            EventInfo param_name,
                            size_t param_value_size,
                            const void *param_value,
                            const size_t *param_value_size_ret)
{
    // CL_INVALID_EVENT if event is a not a valid event object.
    if (!Event::IsValid(event))
    {
        return CL_INVALID_EVENT;
    }

    // CL_INVALID_VALUE if param_name is not valid.
    const cl_version version = event->cast<Event>().getContext().getPlatform().getVersion();
    switch (param_name)
    {
        case EventInfo::Context:
            ANGLE_VALIDATE_VERSION(1, 1);
            break;
        case EventInfo::InvalidEnum:
            return CL_INVALID_VALUE;
        default:
            break;
    }

    return CL_SUCCESS;
}

cl_int ValidateRetainEvent(cl_event event)
{
    // CL_INVALID_EVENT if event is not a valid event object.
    return Event::IsValid(event) ? CL_SUCCESS : CL_INVALID_EVENT;
}

cl_int ValidateReleaseEvent(cl_event event)
{
    // CL_INVALID_EVENT if event is not a valid event object.
    return Event::IsValid(event) ? CL_SUCCESS : CL_INVALID_EVENT;
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

cl_int ValidateEnqueueMapBuffer(cl_command_queue command_queue,
                                cl_mem buffer,
                                cl_bool blocking_map,
                                MapFlags map_flags,
                                size_t offset,
                                size_t size,
                                cl_uint num_events_in_wait_list,
                                const cl_event *event_wait_list,
                                const cl_event *event)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueMapImage(cl_command_queue command_queue,
                               cl_mem image,
                               cl_bool blocking_map,
                               MapFlags map_flags,
                               const size_t *origin,
                               const size_t *region,
                               const size_t *image_row_pitch,
                               const size_t *image_slice_pitch,
                               cl_uint num_events_in_wait_list,
                               const cl_event *event_wait_list,
                               const cl_event *event)
{
    return CL_SUCCESS;
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
                                       CommandQueueProperties properties,
                                       cl_bool enable,
                                       const cl_command_queue_properties *old_properties)
{
    // CL_INVALID_COMMAND_QUEUE if command_queue is not a valid command-queue.
    if (!CommandQueue::IsValid(command_queue))
    {
        return CL_INVALID_COMMAND_QUEUE;
    }

    // CL_INVALID_VALUE if values specified in properties are not valid.
    if (properties.hasOtherBitsThan(CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE |
                                    CL_QUEUE_PROFILING_ENABLE))
    {
        return CL_INVALID_VALUE;
    }

    return CL_SUCCESS;
}

cl_int ValidateCreateImage2D(cl_context context,
                             MemFlags flags,
                             const cl_image_format *image_format,
                             size_t image_width,
                             size_t image_height,
                             size_t image_row_pitch,
                             const void *host_ptr)
{
    const cl_image_desc desc = {CL_MEM_OBJECT_IMAGE2D, image_width, image_height, 0u, 0u,
                                image_row_pitch,       0u,          0u,           0u, {nullptr}};
    return ValidateCreateImage(context, flags, image_format, &desc, host_ptr);
}

cl_int ValidateCreateImage3D(cl_context context,
                             MemFlags flags,
                             const cl_image_format *image_format,
                             size_t image_width,
                             size_t image_height,
                             size_t image_depth,
                             size_t image_row_pitch,
                             size_t image_slice_pitch,
                             const void *host_ptr)
{
    const cl_image_desc desc = {
        CL_MEM_OBJECT_IMAGE3D, image_width,       image_height, image_depth, 0u,
        image_row_pitch,       image_slice_pitch, 0u,           0u,          {nullptr}};
    return ValidateCreateImage(context, flags, image_format, &desc, host_ptr);
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

cl_int ValidateGetExtensionFunctionAddress(const char *func_name)
{
    return func_name != nullptr && *func_name != '\0' ? CL_SUCCESS : CL_INVALID_VALUE;
}

cl_int ValidateCreateCommandQueue(cl_context context,
                                  cl_device_id device,
                                  CommandQueueProperties properties)
{
    // CL_INVALID_CONTEXT if context is not a valid context.
    if (!Context::IsValid(context))
    {
        return CL_INVALID_CONTEXT;
    }

    // CL_INVALID_DEVICE if device is not a valid device or is not associated with context.
    if (!context->cast<Context>().hasDevice(device))
    {
        return CL_INVALID_DEVICE;
    }

    // CL_INVALID_VALUE if values specified in properties are not valid.
    if (properties.hasOtherBitsThan(CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE |
                                    CL_QUEUE_PROFILING_ENABLE))
    {
        return CL_INVALID_VALUE;
    }

    return CL_SUCCESS;
}

cl_int ValidateCreateSampler(cl_context context,
                             cl_bool normalized_coords,
                             AddressingMode addressing_mode,
                             FilterMode filter_mode)
{
    // CL_INVALID_CONTEXT if context is not a valid context.
    if (!Context::IsValid(context))
    {
        return CL_INVALID_CONTEXT;
    }

    // CL_INVALID_VALUE if addressing_mode, filter_mode, normalized_coords
    // or a combination of these arguements are not valid.
    if ((normalized_coords != CL_FALSE && normalized_coords != CL_TRUE) ||
        addressing_mode == AddressingMode::InvalidEnum || filter_mode == FilterMode::InvalidEnum)
    {
        return CL_INVALID_VALUE;
    }

    // CL_INVALID_OPERATION if images are not supported by any device associated with context.
    if (!context->cast<Context>().supportsImages())
    {
        return CL_INVALID_OPERATION;
    }

    return CL_SUCCESS;
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
cl_int ValidateCreateSubBuffer(cl_mem buffer,
                               MemFlags flags,
                               cl_buffer_create_type buffer_create_type,
                               const void *buffer_create_info)
{
    // CL_INVALID_MEM_OBJECT if buffer is not a valid buffer object or is a sub-buffer object.
    if (!Buffer::IsValid(buffer))
    {
        return CL_INVALID_MEM_OBJECT;
    }
    const Buffer &buf = buffer->cast<Buffer>();
    if (buf.isSubBuffer() || !buf.getContext().getPlatform().isVersionOrNewer(1u, 1u))
    {
        return CL_INVALID_MEM_OBJECT;
    }

    if (!ValidateMemoryFlags(flags, buf.getContext().getPlatform()))
    {
        return CL_INVALID_VALUE;
    }

    const MemFlags bufFlags = buf.getFlags();
    // CL_INVALID_VALUE if buffer was created with CL_MEM_WRITE_ONLY
    // and flags specifies CL_MEM_READ_WRITE or CL_MEM_READ_ONLY,
    if ((bufFlags.isSet(CL_MEM_WRITE_ONLY) && flags.isSet(CL_MEM_READ_WRITE | CL_MEM_READ_ONLY)) ||
        // or if buffer was created with CL_MEM_READ_ONLY
        // and flags specifies CL_MEM_READ_WRITE or CL_MEM_WRITE_ONLY,
        (bufFlags.isSet(CL_MEM_READ_ONLY) && flags.isSet(CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY)) ||
        // or if flags specifies CL_MEM_USE_HOST_PTR, CL_MEM_ALLOC_HOST_PTR or CL_MEM_COPY_HOST_PTR.
        flags.isSet(CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR | CL_MEM_COPY_HOST_PTR))
    {
        return CL_INVALID_VALUE;
    }

    // CL_INVALID_VALUE if buffer was created with CL_MEM_HOST_WRITE_ONLY
    // and flags specify CL_MEM_HOST_READ_ONLY,
    if ((bufFlags.isSet(CL_MEM_HOST_WRITE_ONLY) && flags.isSet(CL_MEM_HOST_READ_ONLY)) ||
        // or if buffer was created with CL_MEM_HOST_READ_ONLY
        // and flags specify CL_MEM_HOST_WRITE_ONLY,
        (bufFlags.isSet(CL_MEM_HOST_READ_ONLY) && flags.isSet(CL_MEM_HOST_WRITE_ONLY)) ||
        // or if buffer was created with CL_MEM_HOST_NO_ACCESS
        // and flags specify CL_MEM_HOST_READ_ONLY or CL_MEM_HOST_WRITE_ONLY.
        (bufFlags.isSet(CL_MEM_HOST_NO_ACCESS) &&
         flags.isSet(CL_MEM_HOST_READ_ONLY | CL_MEM_HOST_WRITE_ONLY)))
    {
        return CL_INVALID_VALUE;
    }

    // CL_INVALID_VALUE if the value specified in buffer_create_type is not valid.
    if (buffer_create_type != CL_BUFFER_CREATE_TYPE_REGION)
    {
        return CL_INVALID_VALUE;
    }

    // CL_INVALID_VALUE if value(s) specified in buffer_create_info
    // (for a given buffer_create_type) is not valid or if buffer_create_info is NULL.
    // CL_INVALID_VALUE if the region specified by the cl_buffer_region structure
    // passed in buffer_create_info is out of bounds in buffer.
    const cl_buffer_region *region = static_cast<const cl_buffer_region *>(buffer_create_info);
    if (region == nullptr || !buf.isRegionValid(*region))
    {
        return CL_INVALID_VALUE;
    }

    // CL_INVALID_BUFFER_SIZE if the size field of the cl_buffer_region structure
    // passed in buffer_create_info is 0.
    if (region->size == 0u)
    {
        return CL_INVALID_BUFFER_SIZE;
    }

    return CL_SUCCESS;
}

cl_int ValidateSetMemObjectDestructorCallback(cl_mem memobj,
                                              void(CL_CALLBACK *pfn_notify)(cl_mem memobj,
                                                                            void *user_data),
                                              const void *user_data)
{
    return CL_SUCCESS;
}

cl_int ValidateCreateUserEvent(cl_context context)
{
    // CL_INVALID_CONTEXT if context is not a valid context.
    return Context::IsValidAndVersionOrNewer(context, 1u, 1u) ? CL_SUCCESS : CL_INVALID_CONTEXT;
}

cl_int ValidateSetUserEventStatus(cl_event event, cl_int execution_status)
{
    // CL_INVALID_EVENT if event is not a valid user event object.
    if (!Event::IsValid(event))
    {
        return CL_INVALID_EVENT;
    }
    const Event &evt = event->cast<Event>();
    if (!evt.getContext().getPlatform().isVersionOrNewer(1u, 1u) ||
        evt.getCommandType() != CL_COMMAND_USER)
    {
        return CL_INVALID_EVENT;
    }

    // CL_INVALID_VALUE if the execution_status is not CL_COMPLETE or a negative integer value.
    if (execution_status != CL_COMPLETE && execution_status >= 0)
    {
        return CL_INVALID_VALUE;
    }

    // CL_INVALID_OPERATION if the execution_status for event has already been changed
    // by a previous call to clSetUserEventStatus.
    if (evt.wasStatusChanged())
    {
        return CL_INVALID_OPERATION;
    }

    return CL_SUCCESS;
}

cl_int ValidateSetEventCallback(cl_event event,
                                cl_int command_exec_callback_type,
                                void(CL_CALLBACK *pfn_notify)(cl_event event,
                                                              cl_int event_command_status,
                                                              void *user_data),
                                const void *user_data)
{
    // CL_INVALID_EVENT if event is not a valid event object.
    if (!Event::IsValid(event) ||
        !event->cast<Event>().getContext().getPlatform().isVersionOrNewer(1u, 1u))
    {
        return CL_INVALID_EVENT;
    }

    // CL_INVALID_VALUE if pfn_event_notify is NULL
    // or if command_exec_callback_type is not CL_SUBMITTED, CL_RUNNING, or CL_COMPLETE.
    if (pfn_notify == nullptr ||
        (command_exec_callback_type != CL_SUBMITTED && command_exec_callback_type != CL_RUNNING &&
         command_exec_callback_type != CL_COMPLETE))
    {
        return CL_INVALID_VALUE;
    }

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
    // CL_INVALID_DEVICE if in_device is not a valid device.
    if (!Device::IsValid(in_device))
    {
        return CL_INVALID_DEVICE;
    }
    const Device &device = in_device->cast<Device>();
    if (!device.isVersionOrNewer(1u, 2u))
    {
        return CL_INVALID_DEVICE;
    }

    // CL_INVALID_VALUE if values specified in properties are not valid
    // or if values specified in properties are valid but not supported by the device
    const std::vector<cl_device_partition_property> &devProps =
        device.getInfo().mPartitionProperties;
    if (properties == nullptr ||
        std::find(devProps.cbegin(), devProps.cend(), *properties) == devProps.cend())
    {
        return CL_INVALID_VALUE;
    }

    return CL_SUCCESS;
}

cl_int ValidateRetainDevice(cl_device_id device)
{
    // CL_INVALID_DEVICE if device is not a valid device.
    if (!Device::IsValid(device) || !device->cast<Device>().isVersionOrNewer(1u, 2u))
    {
        return CL_INVALID_DEVICE;
    }
    return CL_SUCCESS;
}

cl_int ValidateReleaseDevice(cl_device_id device)
{
    // CL_INVALID_DEVICE if device is not a valid device.
    if (!Device::IsValid(device) || !device->cast<Device>().isVersionOrNewer(1u, 2u))
    {
        return CL_INVALID_DEVICE;
    }
    return CL_SUCCESS;
}

cl_int ValidateCreateImage(cl_context context,
                           MemFlags flags,
                           const cl_image_format *image_format,
                           const cl_image_desc *image_desc,
                           const void *host_ptr)
{
    // CL_INVALID_CONTEXT if context is not a valid context.
    if (!Context::IsValidAndVersionOrNewer(context, 1u, 2u))
    {
        return CL_INVALID_CONTEXT;
    }
    const Context &ctx = context->cast<Context>();

    // CL_INVALID_VALUE if values specified in flags are not valid.
    if (!ValidateMemoryFlags(flags, ctx.getPlatform()))
    {
        return CL_INVALID_VALUE;
    }

    // CL_INVALID_IMAGE_FORMAT_DESCRIPTOR if values specified in image_format are not valid
    // or if image_format is NULL.
    if (!ValidateImageFormat(image_format, ctx.getPlatform()))
    {
        return CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
    }

    // CL_INVALID_IMAGE_DESCRIPTOR if image_desc is NULL.
    if (image_desc == nullptr)
    {
        return CL_INVALID_IMAGE_DESCRIPTOR;
    }

    const size_t elemSize = GetElementSize(*image_format);
    if (elemSize == 0u)
    {
        ASSERT(false);
        ERR() << "Failed to calculate image element size";
        return CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
    }
    const size_t rowPitch = image_desc->image_row_pitch != 0u ? image_desc->image_row_pitch
                                                              : image_desc->image_width * elemSize;
    const size_t imageHeight =
        image_desc->image_type == CL_MEM_OBJECT_IMAGE1D_ARRAY ? 1u : image_desc->image_height;
    const size_t sliceSize = imageHeight * rowPitch;

    // CL_INVALID_IMAGE_DESCRIPTOR if values specified in image_desc are not valid.
    switch (image_desc->image_type)
    {
        case CL_MEM_OBJECT_IMAGE1D:
            if (image_desc->image_width == 0u)
            {
                return CL_INVALID_IMAGE_DESCRIPTOR;
            }
            break;
        case CL_MEM_OBJECT_IMAGE2D:
            if (image_desc->image_width == 0u || image_desc->image_height == 0u)
            {
                return CL_INVALID_IMAGE_DESCRIPTOR;
            }
            break;
        case CL_MEM_OBJECT_IMAGE3D:
            if (image_desc->image_width == 0u || image_desc->image_height == 0u ||
                image_desc->image_depth == 0u)
            {
                return CL_INVALID_IMAGE_DESCRIPTOR;
            }
            break;
        case CL_MEM_OBJECT_IMAGE1D_ARRAY:
            if (image_desc->image_width == 0u || image_desc->image_array_size == 0u)
            {
                return CL_INVALID_IMAGE_DESCRIPTOR;
            }
            break;
        case CL_MEM_OBJECT_IMAGE2D_ARRAY:
            if (image_desc->image_width == 0u || image_desc->image_height == 0u ||
                image_desc->image_array_size == 0u)
            {
                return CL_INVALID_IMAGE_DESCRIPTOR;
            }
            break;
        case CL_MEM_OBJECT_IMAGE1D_BUFFER:
            if (image_desc->image_width == 0u)
            {
                return CL_INVALID_IMAGE_DESCRIPTOR;
            }
            break;
        default:
            return CL_INVALID_IMAGE_DESCRIPTOR;
    }
    // image_row_pitch must be 0 if host_ptr is NULL and can be either 0
    // or >= image_width * size of element in bytes if host_ptr is not NULL.
    // If image_row_pitch is not 0, it must be a multiple of the image element size in bytes.
    if (image_desc->image_row_pitch != 0u &&
        (host_ptr == nullptr || image_desc->image_row_pitch < image_desc->image_width * elemSize ||
         (image_desc->image_row_pitch % elemSize) != 0u))
    {
        return CL_INVALID_IMAGE_DESCRIPTOR;
    }
    // image_slice_pitch must be 0 if host_ptr is NULL. If host_ptr is not NULL, image_slice_pitch
    // can be either 0 or >= image_row_pitch * image_height for a 2D image array or 3D image
    // and can be either 0 or >= image_row_pitch for a 1D image array.
    // If image_slice_pitch is not 0, it must be a multiple of the image_row_pitch.
    if (image_desc->image_slice_pitch != 0u &&
        (host_ptr == nullptr || image_desc->image_slice_pitch < sliceSize ||
         (image_desc->image_slice_pitch % rowPitch) != 0u))
    {
        return CL_INVALID_IMAGE_DESCRIPTOR;
    }
    // num_mip_levels and num_samples must be 0.
    if (image_desc->num_mip_levels != 0u || image_desc->num_samples != 0u)
    {
        return CL_INVALID_IMAGE_DESCRIPTOR;
    }

    // buffer can be a buffer memory object if image_type is CL_MEM_OBJECT_IMAGE1D_BUFFER or
    // CL_MEM_OBJECT_IMAGE2D. buffer can be an image object if image_type is CL_MEM_OBJECT_IMAGE2D.
    // Otherwise it must be NULL.
    if (image_desc->buffer != nullptr &&
        (!Buffer::IsValid(image_desc->buffer) ||
         (image_desc->image_type != CL_MEM_OBJECT_IMAGE1D_BUFFER &&
          image_desc->image_type != CL_MEM_OBJECT_IMAGE2D)) &&
        (!Image::IsValid(image_desc->buffer) || image_desc->image_type != CL_MEM_OBJECT_IMAGE2D))
    {
        return CL_INVALID_IMAGE_DESCRIPTOR;
    }

    // CL_INVALID_OPERATION if there are no devices in context that support images.
    if (!ctx.supportsImages())
    {
        return CL_INVALID_OPERATION;
    }

    // CL_INVALID_IMAGE_SIZE if image dimensions specified in image_desc exceed the maximum
    // image dimensions described in the Device Queries table for all devices in context.
    const DevicePtrs &devices = ctx.getDevices();
    if (std::find_if(devices.cbegin(), devices.cend(), [&](const DevicePtr &ptr) {
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
                default:
                    ASSERT(false);
                    break;
            }
            return false;
        }) == devices.cend())
    {
        return CL_INVALID_IMAGE_SIZE;
    }

    // CL_INVALID_HOST_PTR
    // if host_ptr is NULL and CL_MEM_USE_HOST_PTR or CL_MEM_COPY_HOST_PTR are set in flags or
    // if host_ptr is not NULL but CL_MEM_COPY_HOST_PTR or CL_MEM_USE_HOST_PTR are not set in flags.
    if ((host_ptr != nullptr) != flags.isSet(CL_MEM_USE_HOST_PTR | CL_MEM_COPY_HOST_PTR))
    {
        return CL_INVALID_HOST_PTR;
    }

    return CL_SUCCESS;
}

cl_int ValidateCreateProgramWithBuiltInKernels(cl_context context,
                                               cl_uint num_devices,
                                               const cl_device_id *device_list,
                                               const char *kernel_names)
{
    // CL_INVALID_CONTEXT if context is not a valid context.
    if (!Context::IsValidAndVersionOrNewer(context, 1u, 2u))
    {
        return CL_INVALID_CONTEXT;
    }
    const Context &ctx = context->cast<Context>();

    // CL_INVALID_VALUE if device_list is NULL or num_devices is zero or if kernel_names is NULL.
    if (device_list == nullptr || num_devices == 0u || kernel_names == nullptr)
    {
        return CL_INVALID_VALUE;
    }

    // CL_INVALID_DEVICE if any device in device_list
    // is not in the list of devices associated with context.
    for (size_t index = 0u; index < num_devices; ++index)
    {
        if (!ctx.hasDevice(device_list[index]))
        {
            return CL_INVALID_DEVICE;
        }
    }

    // CL_INVALID_VALUE if kernel_names contains a kernel name
    // that is not supported by any of the devices in device_list.
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
            return CL_INVALID_VALUE;
        }
        start = end;
    } while (*start++ != '\0');

    return CL_SUCCESS;
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

cl_int ValidateLinkProgram(cl_context context,
                           cl_uint num_devices,
                           const cl_device_id *device_list,
                           const char *options,
                           cl_uint num_input_programs,
                           const cl_program *input_programs,
                           void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
                           const void *user_data)
{
    return CL_SUCCESS;
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
    // CL_INVALID_KERNEL if kernel is a not a valid kernel object.
    if (!Kernel::IsValid(kernel))
    {
        return CL_INVALID_KERNEL;
    }
    const Kernel &krnl = kernel->cast<Kernel>();
    if (!krnl.getProgram().getContext().getPlatform().isVersionOrNewer(1u, 2u))
    {
        return CL_INVALID_KERNEL;
    }

    // CL_INVALID_ARG_INDEX if arg_index is not a valid argument index.
    if (arg_index >= krnl.getInfo().mArgs.size())
    {
        return CL_INVALID_ARG_INDEX;
    }

    // CL_KERNEL_ARG_INFO_NOT_AVAILABLE if the argument information is not available for kernel.
    if (!krnl.getInfo().mArgs[arg_index].isAvailable())
    {
        return CL_KERNEL_ARG_INFO_NOT_AVAILABLE;
    }

    // CL_INVALID_VALUE if param_name is not valid.
    if (param_name == KernelArgInfo::InvalidEnum)
    {
        return CL_INVALID_VALUE;
    }

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
                                        MemMigrationFlags flags,
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

cl_int ValidateGetExtensionFunctionAddressForPlatform(cl_platform_id platform,
                                                      const char *func_name)
{
    if (!Platform::IsValid(platform) || func_name == nullptr || *func_name == '\0')
    {
        return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

// CL 2.0
cl_int ValidateCreateCommandQueueWithProperties(cl_context context,
                                                cl_device_id device,
                                                const cl_queue_properties *properties)
{
    // CL_INVALID_CONTEXT if context is not a valid context.
    if (!Context::IsValidAndVersionOrNewer(context, 2u, 0u))
    {
        return CL_INVALID_CONTEXT;
    }

    // CL_INVALID_DEVICE if device is not a valid device or is not associated with context.
    if (!context->cast<Context>().hasDevice(device) ||
        !device->cast<Device>().isVersionOrNewer(2u, 0u))
    {
        return CL_INVALID_DEVICE;
    }

    // CL_INVALID_VALUE if values specified in properties are not valid.
    if (properties != nullptr)
    {
        bool isQueueOnDevice = false;
        bool hasQueueSize    = false;
        while (*properties != 0)
        {
            switch (*properties++)
            {
                case CL_QUEUE_PROPERTIES:
                {
                    const CommandQueueProperties props(*properties++);
                    const CommandQueueProperties validProps(
                        CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE |
                        CL_QUEUE_ON_DEVICE | CL_QUEUE_ON_DEVICE_DEFAULT);
                    if (props.hasOtherBitsThan(validProps) ||
                        // If CL_QUEUE_ON_DEVICE is set, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE
                        // must also be set.
                        (props.isSet(CL_QUEUE_ON_DEVICE) &&
                         !props.isSet(CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)) ||
                        // CL_QUEUE_ON_DEVICE_DEFAULT can only be used with CL_QUEUE_ON_DEVICE.
                        (props.isSet(CL_QUEUE_ON_DEVICE_DEFAULT) &&
                         !props.isSet(CL_QUEUE_ON_DEVICE)))
                    {
                        return CL_INVALID_VALUE;
                    }
                    isQueueOnDevice = props.isSet(CL_QUEUE_ON_DEVICE);
                    break;
                }
                case CL_QUEUE_SIZE:
                {
                    // CL_QUEUE_SIZE must be a value <= CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE.
                    if (*properties++ > device->cast<Device>().getInfo().mQueueOnDeviceMaxSize)
                    {
                        return CL_INVALID_VALUE;
                    }
                    hasQueueSize = true;
                    break;
                }
                default:
                    return CL_INVALID_VALUE;
            }
        }

        // CL_QUEUE_SIZE can only be specified if CL_QUEUE_ON_DEVICE is set in CL_QUEUE_PROPERTIES.
        if (hasQueueSize && !isQueueOnDevice)
        {
            return CL_INVALID_VALUE;
        }
    }

    return CL_SUCCESS;
}

cl_int ValidateCreatePipe(cl_context context,
                          MemFlags flags,
                          cl_uint pipe_packet_size,
                          cl_uint pipe_max_packets,
                          const cl_pipe_properties *properties)
{
    return CL_SUCCESS;
}

cl_int ValidateGetPipeInfo(cl_mem pipe,
                           PipeInfo param_name,
                           size_t param_value_size,
                           const void *param_value,
                           const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateSVMAlloc(cl_context context, SVM_MemFlags flags, size_t size, cl_uint alignment)
{
    return CL_SUCCESS;
}

cl_int ValidateSVMFree(cl_context context, const void *svm_pointer)
{
    return CL_SUCCESS;
}

cl_int ValidateCreateSamplerWithProperties(cl_context context,
                                           const cl_sampler_properties *sampler_properties)
{
    // CL_INVALID_CONTEXT if context is not a valid context.
    if (!Context::IsValidAndVersionOrNewer(context, 2u, 0u))
    {
        return CL_INVALID_CONTEXT;
    }

    // CL_INVALID_VALUE if the property name in sampler_properties is not a supported property name,
    // if the value specified for a supported property name is not valid,
    // or if the same property name is specified more than once.
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
                        return CL_INVALID_VALUE;
                    }
                    hasNormalizedCoords = true;
                    ++propIt;
                    break;
                case CL_SAMPLER_ADDRESSING_MODE:
                    if (hasAddressingMode || FromCLenum<AddressingMode>(static_cast<CLenum>(
                                                 *propIt++)) == AddressingMode::InvalidEnum)
                    {
                        return CL_INVALID_VALUE;
                    }
                    hasAddressingMode = true;
                    break;
                case CL_SAMPLER_FILTER_MODE:
                    if (hasFilterMode || FromCLenum<FilterMode>(static_cast<CLenum>(*propIt++)) ==
                                             FilterMode::InvalidEnum)
                    {
                        return CL_INVALID_VALUE;
                    }
                    hasFilterMode = true;
                    break;
                default:
                    return CL_INVALID_VALUE;
            }
        }
    }

    // CL_INVALID_OPERATION if images are not supported by any device associated with context.
    if (!context->cast<Context>().supportsImages())
    {
        return CL_INVALID_OPERATION;
    }

    return CL_SUCCESS;
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
                             MapFlags flags,
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

cl_int ValidateCreateProgramWithIL(cl_context context, const void *il, size_t length)
{
    // CL_INVALID_CONTEXT if context is not a valid context.
    if (!Context::IsValidAndVersionOrNewer(context, 2u, 1u))
    {
        return CL_INVALID_CONTEXT;
    }
    const Context &ctx = context->cast<Context>();

    // CL_INVALID_OPERATION if no devices in context support intermediate language programs.
    if (!ctx.supportsIL())
    {
        return CL_INVALID_OPERATION;
    }

    // CL_INVALID_VALUE if il is NULL or if length is zero.
    if (il == nullptr || length == 0u)
    {
        return CL_INVALID_VALUE;
    }

    return CL_SUCCESS;
}

cl_int ValidateCloneKernel(cl_kernel source_kernel)
{
    return CL_SUCCESS;
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
                                    MemMigrationFlags flags,
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

cl_int ValidateCreateBufferWithProperties(cl_context context,
                                          const cl_mem_properties *properties,
                                          MemFlags flags,
                                          size_t size,
                                          const void *host_ptr)
{
    const cl_int errorCode = ValidateCreateBuffer(context, flags, size, host_ptr);
    if (errorCode != CL_SUCCESS)
    {
        return errorCode;
    }

    // CL_INVALID_CONTEXT if context is not a valid context.
    if (!context->cast<Context>().getPlatform().isVersionOrNewer(3u, 0u))
    {
        return CL_INVALID_CONTEXT;
    }

    // CL_INVALID_PROPERTY if a property name in properties is not a supported property name,
    // if the value specified for a supported property name is not valid,
    // or if the same property name is specified more than once.
    if (!ValidateMemoryProperties(properties))
    {
        return CL_INVALID_PROPERTY;
    }

    return CL_SUCCESS;
}

cl_int ValidateCreateImageWithProperties(cl_context context,
                                         const cl_mem_properties *properties,
                                         MemFlags flags,
                                         const cl_image_format *image_format,
                                         const cl_image_desc *image_desc,
                                         const void *host_ptr)
{
    const cl_int errorCode =
        ValidateCreateImage(context, flags, image_format, image_desc, host_ptr);
    if (errorCode != CL_SUCCESS)
    {
        return errorCode;
    }

    // CL_INVALID_CONTEXT if context is not a valid context.
    if (!context->cast<Context>().getPlatform().isVersionOrNewer(3u, 0u))
    {
        return CL_INVALID_CONTEXT;
    }

    // CL_INVALID_PROPERTY if a property name in properties is not a supported property name,
    // if the value specified for a supported property name is not valid,
    // or if the same property name is specified more than once.
    if (!ValidateMemoryProperties(properties))
    {
        return CL_INVALID_PROPERTY;
    }

    return CL_SUCCESS;
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
