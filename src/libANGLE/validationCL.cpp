//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// validationCL.cpp: Validation functions for generic CL entry point parameters

#include "libANGLE/validationCL_autogen.h"

namespace cl
{
// CL 1.0
cl_int ValidateGetPlatformIDs(cl_uint num_entries,
                              Platform *const *platformsPacked,
                              const cl_uint *num_platforms)
{
    if ((num_entries == 0u && platformsPacked != nullptr) ||
        (platformsPacked == nullptr && num_platforms == nullptr))
    {
        return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

cl_int ValidateGetPlatformInfo(const Platform *platformPacked,
                               PlatformInfo param_namePacked,
                               size_t param_value_size,
                               const void *param_value,
                               const size_t *param_value_size_ret)
{
    if (!Platform::IsValid(platformPacked))
    {
        return CL_INVALID_PLATFORM;
    }
    if (param_namePacked == PlatformInfo::InvalidEnum ||
        (param_value_size == 0u && param_value != nullptr))
    {
        return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

cl_int ValidateGetDeviceIDs(const Platform *platformPacked,
                            cl_device_type device_type,
                            cl_uint num_entries,
                            Device *const *devicesPacked,
                            const cl_uint *num_devices)
{
    return CL_SUCCESS;
}

cl_int ValidateGetDeviceInfo(const Device *devicePacked,
                             DeviceInfo param_namePacked,
                             size_t param_value_size,
                             const void *param_value,
                             const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

bool ValidateCreateContext(const cl_context_properties *properties,
                           cl_uint num_devices,
                           Device *const *devicesPacked,
                           void(CL_CALLBACK *pfn_notify)(const char *errinfo,
                                                         const void *private_info,
                                                         size_t cb,
                                                         void *user_data),
                           const void *user_data,
                           cl_int *errcode_ret)
{
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
    return true;
}

cl_int ValidateRetainContext(const Context *contextPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateReleaseContext(const Context *contextPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateGetContextInfo(const Context *contextPacked,
                              ContextInfo param_namePacked,
                              size_t param_value_size,
                              const void *param_value,
                              const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateRetainCommandQueue(const CommandQueue *command_queuePacked)
{
    return CL_SUCCESS;
}

cl_int ValidateReleaseCommandQueue(const CommandQueue *command_queuePacked)
{
    return CL_SUCCESS;
}

cl_int ValidateGetCommandQueueInfo(const CommandQueue *command_queuePacked,
                                   CommandQueueInfo param_namePacked,
                                   size_t param_value_size,
                                   const void *param_value,
                                   const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

bool ValidateCreateBuffer(const Context *contextPacked,
                          cl_mem_flags flags,
                          size_t size,
                          const void *host_ptr,
                          cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateRetainMemObject(const Memory *memobjPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateReleaseMemObject(const Memory *memobjPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateGetSupportedImageFormats(const Context *contextPacked,
                                        cl_mem_flags flags,
                                        MemObjectType image_typePacked,
                                        cl_uint num_entries,
                                        const cl_image_format *image_formats,
                                        const cl_uint *num_image_formats)
{
    return CL_SUCCESS;
}

cl_int ValidateGetMemObjectInfo(const Memory *memobjPacked,
                                MemInfo param_namePacked,
                                size_t param_value_size,
                                const void *param_value,
                                const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateGetImageInfo(const Memory *imagePacked,
                            ImageInfo param_namePacked,
                            size_t param_value_size,
                            const void *param_value,
                            const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateRetainSampler(const Sampler *samplerPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateReleaseSampler(const Sampler *samplerPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateGetSamplerInfo(const Sampler *samplerPacked,
                              SamplerInfo param_namePacked,
                              size_t param_value_size,
                              const void *param_value,
                              const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

bool ValidateCreateProgramWithSource(const Context *contextPacked,
                                     cl_uint count,
                                     const char **strings,
                                     const size_t *lengths,
                                     cl_int *errcode_ret)
{
    return true;
}

bool ValidateCreateProgramWithBinary(const Context *contextPacked,
                                     cl_uint num_devices,
                                     Device *const *device_listPacked,
                                     const size_t *lengths,
                                     const unsigned char **binaries,
                                     const cl_int *binary_status,
                                     cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateRetainProgram(const Program *programPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateReleaseProgram(const Program *programPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateBuildProgram(const Program *programPacked,
                            cl_uint num_devices,
                            Device *const *device_listPacked,
                            const char *options,
                            void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
                            const void *user_data)
{
    return CL_SUCCESS;
}

cl_int ValidateGetProgramInfo(const Program *programPacked,
                              ProgramInfo param_namePacked,
                              size_t param_value_size,
                              const void *param_value,
                              const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateGetProgramBuildInfo(const Program *programPacked,
                                   const Device *devicePacked,
                                   ProgramBuildInfo param_namePacked,
                                   size_t param_value_size,
                                   const void *param_value,
                                   const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

bool ValidateCreateKernel(const Program *programPacked,
                          const char *kernel_name,
                          cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateCreateKernelsInProgram(const Program *programPacked,
                                      cl_uint num_kernels,
                                      Kernel *const *kernelsPacked,
                                      const cl_uint *num_kernels_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateRetainKernel(const Kernel *kernelPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateReleaseKernel(const Kernel *kernelPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateSetKernelArg(const Kernel *kernelPacked,
                            cl_uint arg_index,
                            size_t arg_size,
                            const void *arg_value)
{
    return CL_SUCCESS;
}

cl_int ValidateGetKernelInfo(const Kernel *kernelPacked,
                             KernelInfo param_namePacked,
                             size_t param_value_size,
                             const void *param_value,
                             const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateGetKernelWorkGroupInfo(const Kernel *kernelPacked,
                                      const Device *devicePacked,
                                      KernelWorkGroupInfo param_namePacked,
                                      size_t param_value_size,
                                      const void *param_value,
                                      const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateWaitForEvents(cl_uint num_events, Event *const *event_listPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateGetEventInfo(const Event *eventPacked,
                            EventInfo param_namePacked,
                            size_t param_value_size,
                            const void *param_value,
                            const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateRetainEvent(const Event *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateReleaseEvent(const Event *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateGetEventProfilingInfo(const Event *eventPacked,
                                     ProfilingInfo param_namePacked,
                                     size_t param_value_size,
                                     const void *param_value,
                                     const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateFlush(const CommandQueue *command_queuePacked)
{
    return CL_SUCCESS;
}

cl_int ValidateFinish(const CommandQueue *command_queuePacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueReadBuffer(const CommandQueue *command_queuePacked,
                                 const Memory *bufferPacked,
                                 cl_bool blocking_read,
                                 size_t offset,
                                 size_t size,
                                 const void *ptr,
                                 cl_uint num_events_in_wait_list,
                                 Event *const *event_wait_listPacked,
                                 Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueWriteBuffer(const CommandQueue *command_queuePacked,
                                  const Memory *bufferPacked,
                                  cl_bool blocking_write,
                                  size_t offset,
                                  size_t size,
                                  const void *ptr,
                                  cl_uint num_events_in_wait_list,
                                  Event *const *event_wait_listPacked,
                                  Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueCopyBuffer(const CommandQueue *command_queuePacked,
                                 const Memory *src_bufferPacked,
                                 const Memory *dst_bufferPacked,
                                 size_t src_offset,
                                 size_t dst_offset,
                                 size_t size,
                                 cl_uint num_events_in_wait_list,
                                 Event *const *event_wait_listPacked,
                                 Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueReadImage(const CommandQueue *command_queuePacked,
                                const Memory *imagePacked,
                                cl_bool blocking_read,
                                const size_t *origin,
                                const size_t *region,
                                size_t row_pitch,
                                size_t slice_pitch,
                                const void *ptr,
                                cl_uint num_events_in_wait_list,
                                Event *const *event_wait_listPacked,
                                Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueWriteImage(const CommandQueue *command_queuePacked,
                                 const Memory *imagePacked,
                                 cl_bool blocking_write,
                                 const size_t *origin,
                                 const size_t *region,
                                 size_t input_row_pitch,
                                 size_t input_slice_pitch,
                                 const void *ptr,
                                 cl_uint num_events_in_wait_list,
                                 Event *const *event_wait_listPacked,
                                 Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueCopyImage(const CommandQueue *command_queuePacked,
                                const Memory *src_imagePacked,
                                const Memory *dst_imagePacked,
                                const size_t *src_origin,
                                const size_t *dst_origin,
                                const size_t *region,
                                cl_uint num_events_in_wait_list,
                                Event *const *event_wait_listPacked,
                                Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueCopyImageToBuffer(const CommandQueue *command_queuePacked,
                                        const Memory *src_imagePacked,
                                        const Memory *dst_bufferPacked,
                                        const size_t *src_origin,
                                        const size_t *region,
                                        size_t dst_offset,
                                        cl_uint num_events_in_wait_list,
                                        Event *const *event_wait_listPacked,
                                        Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueCopyBufferToImage(const CommandQueue *command_queuePacked,
                                        const Memory *src_bufferPacked,
                                        const Memory *dst_imagePacked,
                                        size_t src_offset,
                                        const size_t *dst_origin,
                                        const size_t *region,
                                        cl_uint num_events_in_wait_list,
                                        Event *const *event_wait_listPacked,
                                        Event *const *eventPacked)
{
    return CL_SUCCESS;
}

bool ValidateEnqueueMapBuffer(const CommandQueue *command_queuePacked,
                              const Memory *bufferPacked,
                              cl_bool blocking_map,
                              cl_map_flags map_flags,
                              size_t offset,
                              size_t size,
                              cl_uint num_events_in_wait_list,
                              Event *const *event_wait_listPacked,
                              Event *const *eventPacked,
                              cl_int *errcode_ret)
{
    return true;
}

bool ValidateEnqueueMapImage(const CommandQueue *command_queuePacked,
                             const Memory *imagePacked,
                             cl_bool blocking_map,
                             cl_map_flags map_flags,
                             const size_t *origin,
                             const size_t *region,
                             const size_t *image_row_pitch,
                             const size_t *image_slice_pitch,
                             cl_uint num_events_in_wait_list,
                             Event *const *event_wait_listPacked,
                             Event *const *eventPacked,
                             cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateEnqueueUnmapMemObject(const CommandQueue *command_queuePacked,
                                     const Memory *memobjPacked,
                                     const void *mapped_ptr,
                                     cl_uint num_events_in_wait_list,
                                     Event *const *event_wait_listPacked,
                                     Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueNDRangeKernel(const CommandQueue *command_queuePacked,
                                    const Kernel *kernelPacked,
                                    cl_uint work_dim,
                                    const size_t *global_work_offset,
                                    const size_t *global_work_size,
                                    const size_t *local_work_size,
                                    cl_uint num_events_in_wait_list,
                                    Event *const *event_wait_listPacked,
                                    Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueNativeKernel(const CommandQueue *command_queuePacked,
                                   void(CL_CALLBACK *user_func)(void *),
                                   const void *args,
                                   size_t cb_args,
                                   cl_uint num_mem_objects,
                                   Memory *const *mem_listPacked,
                                   const void **args_mem_loc,
                                   cl_uint num_events_in_wait_list,
                                   Event *const *event_wait_listPacked,
                                   Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateSetCommandQueueProperty(const CommandQueue *command_queuePacked,
                                       cl_command_queue_properties properties,
                                       cl_bool enable,
                                       const cl_command_queue_properties *old_properties)
{
    return CL_SUCCESS;
}

bool ValidateCreateImage2D(const Context *contextPacked,
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

bool ValidateCreateImage3D(const Context *contextPacked,
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

cl_int ValidateEnqueueMarker(const CommandQueue *command_queuePacked, Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueWaitForEvents(const CommandQueue *command_queuePacked,
                                    cl_uint num_events,
                                    Event *const *event_listPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueBarrier(const CommandQueue *command_queuePacked)
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

bool ValidateCreateCommandQueue(const Context *contextPacked,
                                const Device *devicePacked,
                                cl_command_queue_properties properties,
                                cl_int *errcode_ret)
{
    return true;
}

bool ValidateCreateSampler(const Context *contextPacked,
                           cl_bool normalized_coords,
                           AddressingMode addressing_modePacked,
                           FilterMode filter_modePacked,
                           cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateEnqueueTask(const CommandQueue *command_queuePacked,
                           const Kernel *kernelPacked,
                           cl_uint num_events_in_wait_list,
                           Event *const *event_wait_listPacked,
                           Event *const *eventPacked)
{
    return CL_SUCCESS;
}

// CL 1.1
bool ValidateCreateSubBuffer(const Memory *bufferPacked,
                             cl_mem_flags flags,
                             cl_buffer_create_type buffer_create_type,
                             const void *buffer_create_info,
                             cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateSetMemObjectDestructorCallback(const Memory *memobjPacked,
                                              void(CL_CALLBACK *pfn_notify)(cl_mem memobj,
                                                                            void *user_data),
                                              const void *user_data)
{
    return CL_SUCCESS;
}

bool ValidateCreateUserEvent(const Context *contextPacked, cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateSetUserEventStatus(const Event *eventPacked, cl_int execution_status)
{
    return CL_SUCCESS;
}

cl_int ValidateSetEventCallback(const Event *eventPacked,
                                cl_int command_exec_callback_type,
                                void(CL_CALLBACK *pfn_notify)(cl_event event,
                                                              cl_int event_command_status,
                                                              void *user_data),
                                const void *user_data)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueReadBufferRect(const CommandQueue *command_queuePacked,
                                     const Memory *bufferPacked,
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
                                     Event *const *event_wait_listPacked,
                                     Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueWriteBufferRect(const CommandQueue *command_queuePacked,
                                      const Memory *bufferPacked,
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
                                      Event *const *event_wait_listPacked,
                                      Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueCopyBufferRect(const CommandQueue *command_queuePacked,
                                     const Memory *src_bufferPacked,
                                     const Memory *dst_bufferPacked,
                                     const size_t *src_origin,
                                     const size_t *dst_origin,
                                     const size_t *region,
                                     size_t src_row_pitch,
                                     size_t src_slice_pitch,
                                     size_t dst_row_pitch,
                                     size_t dst_slice_pitch,
                                     cl_uint num_events_in_wait_list,
                                     Event *const *event_wait_listPacked,
                                     Event *const *eventPacked)
{
    return CL_SUCCESS;
}

// CL 1.2
cl_int ValidateCreateSubDevices(const Device *in_devicePacked,
                                const cl_device_partition_property *properties,
                                cl_uint num_devices,
                                Device *const *out_devicesPacked,
                                const cl_uint *num_devices_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateRetainDevice(const Device *devicePacked)
{
    return CL_SUCCESS;
}

cl_int ValidateReleaseDevice(const Device *devicePacked)
{
    return CL_SUCCESS;
}

bool ValidateCreateImage(const Context *contextPacked,
                         cl_mem_flags flags,
                         const cl_image_format *image_format,
                         const cl_image_desc *image_desc,
                         const void *host_ptr,
                         cl_int *errcode_ret)
{
    return true;
}

bool ValidateCreateProgramWithBuiltInKernels(const Context *contextPacked,
                                             cl_uint num_devices,
                                             Device *const *device_listPacked,
                                             const char *kernel_names,
                                             cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateCompileProgram(const Program *programPacked,
                              cl_uint num_devices,
                              Device *const *device_listPacked,
                              const char *options,
                              cl_uint num_input_headers,
                              Program *const *input_headersPacked,
                              const char **header_include_names,
                              void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
                              const void *user_data)
{
    return CL_SUCCESS;
}

bool ValidateLinkProgram(const Context *contextPacked,
                         cl_uint num_devices,
                         Device *const *device_listPacked,
                         const char *options,
                         cl_uint num_input_programs,
                         Program *const *input_programsPacked,
                         void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
                         const void *user_data,
                         cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateUnloadPlatformCompiler(const Platform *platformPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateGetKernelArgInfo(const Kernel *kernelPacked,
                                cl_uint arg_index,
                                KernelArgInfo param_namePacked,
                                size_t param_value_size,
                                const void *param_value,
                                const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueFillBuffer(const CommandQueue *command_queuePacked,
                                 const Memory *bufferPacked,
                                 const void *pattern,
                                 size_t pattern_size,
                                 size_t offset,
                                 size_t size,
                                 cl_uint num_events_in_wait_list,
                                 Event *const *event_wait_listPacked,
                                 Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueFillImage(const CommandQueue *command_queuePacked,
                                const Memory *imagePacked,
                                const void *fill_color,
                                const size_t *origin,
                                const size_t *region,
                                cl_uint num_events_in_wait_list,
                                Event *const *event_wait_listPacked,
                                Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueMigrateMemObjects(const CommandQueue *command_queuePacked,
                                        cl_uint num_mem_objects,
                                        Memory *const *mem_objectsPacked,
                                        cl_mem_migration_flags flags,
                                        cl_uint num_events_in_wait_list,
                                        Event *const *event_wait_listPacked,
                                        Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueMarkerWithWaitList(const CommandQueue *command_queuePacked,
                                         cl_uint num_events_in_wait_list,
                                         Event *const *event_wait_listPacked,
                                         Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueBarrierWithWaitList(const CommandQueue *command_queuePacked,
                                          cl_uint num_events_in_wait_list,
                                          Event *const *event_wait_listPacked,
                                          Event *const *eventPacked)
{
    return CL_SUCCESS;
}

bool ValidateGetExtensionFunctionAddressForPlatform(const Platform *platformPacked,
                                                    const char *func_name)
{
    return Platform::IsValid(platformPacked) && func_name != nullptr && *func_name != '\0';
}

// CL 2.0
bool ValidateCreateCommandQueueWithProperties(const Context *contextPacked,
                                              const Device *devicePacked,
                                              const cl_queue_properties *properties,
                                              cl_int *errcode_ret)
{
    return true;
}

bool ValidateCreatePipe(const Context *contextPacked,
                        cl_mem_flags flags,
                        cl_uint pipe_packet_size,
                        cl_uint pipe_max_packets,
                        const cl_pipe_properties *properties,
                        cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateGetPipeInfo(const Memory *pipePacked,
                           PipeInfo param_namePacked,
                           size_t param_value_size,
                           const void *param_value,
                           const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

bool ValidateSVMAlloc(const Context *contextPacked,
                      cl_svm_mem_flags flags,
                      size_t size,
                      cl_uint alignment)
{
    return true;
}

bool ValidateSVMFree(const Context *contextPacked, const void *svm_pointer)
{
    return true;
}

bool ValidateCreateSamplerWithProperties(const Context *contextPacked,
                                         const cl_sampler_properties *sampler_properties,
                                         cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateSetKernelArgSVMPointer(const Kernel *kernelPacked,
                                      cl_uint arg_index,
                                      const void *arg_value)
{
    return CL_SUCCESS;
}

cl_int ValidateSetKernelExecInfo(const Kernel *kernelPacked,
                                 KernelExecInfo param_namePacked,
                                 size_t param_value_size,
                                 const void *param_value)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueSVMFree(const CommandQueue *command_queuePacked,
                              cl_uint num_svm_pointers,
                              void *const svm_pointers[],
                              void(CL_CALLBACK *pfn_free_func)(cl_command_queue queue,
                                                               cl_uint num_svm_pointers,
                                                               void *svm_pointers[],
                                                               void *user_data),
                              const void *user_data,
                              cl_uint num_events_in_wait_list,
                              Event *const *event_wait_listPacked,
                              Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueSVMMemcpy(const CommandQueue *command_queuePacked,
                                cl_bool blocking_copy,
                                const void *dst_ptr,
                                const void *src_ptr,
                                size_t size,
                                cl_uint num_events_in_wait_list,
                                Event *const *event_wait_listPacked,
                                Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueSVMMemFill(const CommandQueue *command_queuePacked,
                                 const void *svm_ptr,
                                 const void *pattern,
                                 size_t pattern_size,
                                 size_t size,
                                 cl_uint num_events_in_wait_list,
                                 Event *const *event_wait_listPacked,
                                 Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueSVMMap(const CommandQueue *command_queuePacked,
                             cl_bool blocking_map,
                             cl_map_flags flags,
                             const void *svm_ptr,
                             size_t size,
                             cl_uint num_events_in_wait_list,
                             Event *const *event_wait_listPacked,
                             Event *const *eventPacked)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueSVMUnmap(const CommandQueue *command_queuePacked,
                               const void *svm_ptr,
                               cl_uint num_events_in_wait_list,
                               Event *const *event_wait_listPacked,
                               Event *const *eventPacked)
{
    return CL_SUCCESS;
}

// CL 2.1
cl_int ValidateSetDefaultDeviceCommandQueue(const Context *contextPacked,
                                            const Device *devicePacked,
                                            const CommandQueue *command_queuePacked)
{
    return CL_SUCCESS;
}

cl_int ValidateGetDeviceAndHostTimer(const Device *devicePacked,
                                     const cl_ulong *device_timestamp,
                                     const cl_ulong *host_timestamp)
{
    return CL_SUCCESS;
}

cl_int ValidateGetHostTimer(const Device *devicePacked, const cl_ulong *host_timestamp)
{
    return CL_SUCCESS;
}

bool ValidateCreateProgramWithIL(const Context *contextPacked,
                                 const void *il,
                                 size_t length,
                                 cl_int *errcode_ret)
{
    return true;
}

bool ValidateCloneKernel(const Kernel *source_kernelPacked, cl_int *errcode_ret)
{
    return true;
}

cl_int ValidateGetKernelSubGroupInfo(const Kernel *kernelPacked,
                                     const Device *devicePacked,
                                     KernelSubGroupInfo param_namePacked,
                                     size_t input_value_size,
                                     const void *input_value,
                                     size_t param_value_size,
                                     const void *param_value,
                                     const size_t *param_value_size_ret)
{
    return CL_SUCCESS;
}

cl_int ValidateEnqueueSVMMigrateMem(const CommandQueue *command_queuePacked,
                                    cl_uint num_svm_pointers,
                                    const void **svm_pointers,
                                    const size_t *sizes,
                                    cl_mem_migration_flags flags,
                                    cl_uint num_events_in_wait_list,
                                    Event *const *event_wait_listPacked,
                                    Event *const *eventPacked)
{
    return CL_SUCCESS;
}

// CL 2.2
cl_int ValidateSetProgramReleaseCallback(const Program *programPacked,
                                         void(CL_CALLBACK *pfn_notify)(cl_program program,
                                                                       void *user_data),
                                         const void *user_data)
{
    return CL_SUCCESS;
}

cl_int ValidateSetProgramSpecializationConstant(const Program *programPacked,
                                                cl_uint spec_id,
                                                size_t spec_size,
                                                const void *spec_value)
{
    return CL_SUCCESS;
}

// CL 3.0
cl_int ValidateSetContextDestructorCallback(const Context *contextPacked,
                                            void(CL_CALLBACK *pfn_notify)(cl_context context,
                                                                          void *user_data),
                                            const void *user_data)
{
    return CL_SUCCESS;
}

bool ValidateCreateBufferWithProperties(const Context *contextPacked,
                                        const cl_mem_properties *properties,
                                        cl_mem_flags flags,
                                        size_t size,
                                        const void *host_ptr,
                                        cl_int *errcode_ret)
{
    return true;
}

bool ValidateCreateImageWithProperties(const Context *contextPacked,
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
                                    Platform *const *platformsPacked,
                                    const cl_uint *num_platforms)
{
    if ((num_entries == 0u && platformsPacked != nullptr) ||
        (platformsPacked == nullptr && num_platforms == nullptr))
    {
        return CL_INVALID_VALUE;
    }
    return CL_SUCCESS;
}

}  // namespace cl
