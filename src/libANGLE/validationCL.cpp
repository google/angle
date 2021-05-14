//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// validationCL.cpp: Validation functions for generic CL entry point parameters

#include "libANGLE/validationCL_autogen.h"

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
    if (param_name == PlatformInfo::InvalidEnum ||
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
    return CL_SUCCESS;
}

cl_int ValidateReleaseCommandQueue(cl_command_queue command_queue)
{
    return CL_SUCCESS;
}

cl_int ValidateGetCommandQueueInfo(cl_command_queue command_queue,
                                   CommandQueueInfo param_name,
                                   size_t param_value_size,
                                   const void *param_value,
                                   const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

bool ValidateCreateBuffer(cl_context context,
                          cl_mem_flags flags,
                          size_t size,
                          const void *host_ptr,
                          cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateRetainMemObject(cl_mem memobj)
{
    return CL_SUCCESS;
}

cl_int ValidateReleaseMemObject(cl_mem memobj)
{
    return CL_SUCCESS;
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
    return CL_SUCCESS;
}

cl_int ValidateGetImageInfo(cl_mem image,
                            ImageInfo param_name,
                            size_t param_value_size,
                            const void *param_value,
                            const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateRetainSampler(cl_sampler sampler)
{
    return CL_SUCCESS;
}

cl_int ValidateReleaseSampler(cl_sampler sampler)
{
    return CL_SUCCESS;
}

cl_int ValidateGetSamplerInfo(cl_sampler sampler,
                              SamplerInfo param_name,
                              size_t param_value_size,
                              const void *param_value,
                              const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

bool ValidateCreateProgramWithSource(cl_context context,
                                     cl_uint count,
                                     const char **strings,
                                     const size_t *lengths,
                                     cl_int *errcode_ret)
{
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
    return true;
}

cl_int ValidateRetainProgram(cl_program program)
{
    return CL_SUCCESS;
}

cl_int ValidateReleaseProgram(cl_program program)
{
    return CL_SUCCESS;
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
    return true;
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
    return true;
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
    return true;
}

bool ValidateCreateSampler(cl_context context,
                           cl_bool normalized_coords,
                           AddressingMode addressing_mode,
                           FilterMode filter_mode,
                           cl_int *errcode_ret)
{
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
    if (!Device::IsValid(in_device))
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
    return Device::IsValid(device) ? CL_SUCCESS : CL_INVALID_DEVICE;
}

cl_int ValidateReleaseDevice(cl_device_id device)
{
    return Device::IsValid(device) ? CL_SUCCESS : CL_INVALID_DEVICE;
}

bool ValidateCreateImage(cl_context context,
                         cl_mem_flags flags,
                         const cl_image_format *image_format,
                         const cl_image_desc *image_desc,
                         const void *host_ptr,
                         cl_int *errcode_ret)
{
    return true;
}

bool ValidateCreateProgramWithBuiltInKernels(cl_context context,
                                             cl_uint num_devices,
                                             const cl_device_id *device_list,
                                             const char *kernel_names,
                                             cl_int *errcode_ret)
{
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
