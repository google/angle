//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// cl_stubs.cpp: Stubs for CL entry points.
//

#include "libGLESv2/cl_stubs_autogen.h"

namespace cl
{
cl_int GetPlatformIDs(cl_uint num_entries, Platform **platforms, cl_uint *num_platforms)
{
    return 0;
}

cl_int GetPlatformInfo(Platform *platform,
                       PlatformInfo param_name,
                       size_t param_value_size,
                       void *param_value,
                       size_t *param_value_size_ret)
{
    return 0;
}

cl_int GetDeviceIDs(Platform *platform,
                    cl_device_type device_type,
                    cl_uint num_entries,
                    Device **devices,
                    cl_uint *num_devices)
{
    return 0;
}

cl_int GetDeviceInfo(Device *device,
                     DeviceInfo param_name,
                     size_t param_value_size,
                     void *param_value,
                     size_t *param_value_size_ret)
{
    return 0;
}

cl_int CreateSubDevices(Device *in_device,
                        const cl_device_partition_property *properties,
                        cl_uint num_devices,
                        Device **out_devices,
                        cl_uint *num_devices_ret)
{
    return 0;
}

cl_int RetainDevice(Device *device)
{
    return 0;
}

cl_int ReleaseDevice(Device *device)
{
    return 0;
}

cl_int SetDefaultDeviceCommandQueue(Context *context, Device *device, CommandQueue *command_queue)
{
    return 0;
}

cl_int GetDeviceAndHostTimer(Device *device, cl_ulong *device_timestamp, cl_ulong *host_timestamp)
{
    return 0;
}

cl_int GetHostTimer(Device *device, cl_ulong *host_timestamp)
{
    return 0;
}

Context *CreateContext(const cl_context_properties *properties,
                       cl_uint num_devices,
                       Device *const *devices,
                       void(CL_CALLBACK *pfn_notify)(const char *errinfo,
                                                     const void *private_info,
                                                     size_t cb,
                                                     void *user_data),
                       void *user_data,
                       cl_int *errcode_ret)
{
    return 0;
}

Context *CreateContextFromType(const cl_context_properties *properties,
                               cl_device_type device_type,
                               void(CL_CALLBACK *pfn_notify)(const char *errinfo,
                                                             const void *private_info,
                                                             size_t cb,
                                                             void *user_data),
                               void *user_data,
                               cl_int *errcode_ret)
{
    return 0;
}

cl_int RetainContext(Context *context)
{
    return 0;
}

cl_int ReleaseContext(Context *context)
{
    return 0;
}

cl_int GetContextInfo(Context *context,
                      ContextInfo param_name,
                      size_t param_value_size,
                      void *param_value,
                      size_t *param_value_size_ret)
{
    return 0;
}

cl_int SetContextDestructorCallback(Context *context,
                                    void(CL_CALLBACK *pfn_notify)(cl_context context,
                                                                  void *user_data),
                                    void *user_data)
{
    return 0;
}

CommandQueue *CreateCommandQueueWithProperties(Context *context,
                                               Device *device,
                                               const cl_queue_properties *properties,
                                               cl_int *errcode_ret)
{
    return 0;
}

cl_int RetainCommandQueue(CommandQueue *command_queue)
{
    return 0;
}

cl_int ReleaseCommandQueue(CommandQueue *command_queue)
{
    return 0;
}

cl_int GetCommandQueueInfo(CommandQueue *command_queue,
                           CommandQueueInfo param_name,
                           size_t param_value_size,
                           void *param_value,
                           size_t *param_value_size_ret)
{
    return 0;
}

Memory *CreateBuffer(Context *context,
                     cl_mem_flags flags,
                     size_t size,
                     void *host_ptr,
                     cl_int *errcode_ret)
{
    return 0;
}

Memory *CreateBufferWithProperties(Context *context,
                                   const cl_mem_properties *properties,
                                   cl_mem_flags flags,
                                   size_t size,
                                   void *host_ptr,
                                   cl_int *errcode_ret)
{
    return 0;
}

Memory *CreateSubBuffer(Memory *buffer,
                        cl_mem_flags flags,
                        cl_buffer_create_type buffer_create_type,
                        const void *buffer_create_info,
                        cl_int *errcode_ret)
{
    return 0;
}

Memory *CreateImage(Context *context,
                    cl_mem_flags flags,
                    const cl_image_format *image_format,
                    const cl_image_desc *image_desc,
                    void *host_ptr,
                    cl_int *errcode_ret)
{
    return 0;
}

Memory *CreateImageWithProperties(Context *context,
                                  const cl_mem_properties *properties,
                                  cl_mem_flags flags,
                                  const cl_image_format *image_format,
                                  const cl_image_desc *image_desc,
                                  void *host_ptr,
                                  cl_int *errcode_ret)
{
    return 0;
}

Memory *CreatePipe(Context *context,
                   cl_mem_flags flags,
                   cl_uint pipe_packet_size,
                   cl_uint pipe_max_packets,
                   const cl_pipe_properties *properties,
                   cl_int *errcode_ret)
{
    return 0;
}

cl_int RetainMemObject(Memory *memobj)
{
    return 0;
}

cl_int ReleaseMemObject(Memory *memobj)
{
    return 0;
}

cl_int GetSupportedImageFormats(Context *context,
                                cl_mem_flags flags,
                                MemObjectType image_type,
                                cl_uint num_entries,
                                cl_image_format *image_formats,
                                cl_uint *num_image_formats)
{
    return 0;
}

cl_int GetMemObjectInfo(Memory *memobj,
                        MemInfo param_name,
                        size_t param_value_size,
                        void *param_value,
                        size_t *param_value_size_ret)
{
    return 0;
}

cl_int GetImageInfo(Memory *image,
                    ImageInfo param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret)
{
    return 0;
}

cl_int GetPipeInfo(Memory *pipe,
                   PipeInfo param_name,
                   size_t param_value_size,
                   void *param_value,
                   size_t *param_value_size_ret)
{
    return 0;
}

cl_int SetMemObjectDestructorCallback(Memory *memobj,
                                      void(CL_CALLBACK *pfn_notify)(cl_mem memobj, void *user_data),
                                      void *user_data)
{
    return 0;
}

void *SVMAlloc(Context *context, cl_svm_mem_flags flags, size_t size, cl_uint alignment)
{
    return 0;
}

void SVMFree(Context *context, void *svm_pointer) {}

Sampler *CreateSamplerWithProperties(Context *context,
                                     const cl_sampler_properties *sampler_properties,
                                     cl_int *errcode_ret)
{
    return 0;
}

cl_int RetainSampler(Sampler *sampler)
{
    return 0;
}

cl_int ReleaseSampler(Sampler *sampler)
{
    return 0;
}

cl_int GetSamplerInfo(Sampler *sampler,
                      SamplerInfo param_name,
                      size_t param_value_size,
                      void *param_value,
                      size_t *param_value_size_ret)
{
    return 0;
}

Program *CreateProgramWithSource(Context *context,
                                 cl_uint count,
                                 const char **strings,
                                 const size_t *lengths,
                                 cl_int *errcode_ret)
{
    return 0;
}

Program *CreateProgramWithBinary(Context *context,
                                 cl_uint num_devices,
                                 Device *const *device_list,
                                 const size_t *lengths,
                                 const unsigned char **binaries,
                                 cl_int *binary_status,
                                 cl_int *errcode_ret)
{
    return 0;
}

Program *CreateProgramWithBuiltInKernels(Context *context,
                                         cl_uint num_devices,
                                         Device *const *device_list,
                                         const char *kernel_names,
                                         cl_int *errcode_ret)
{
    return 0;
}

Program *CreateProgramWithIL(Context *context, const void *il, size_t length, cl_int *errcode_ret)
{
    return 0;
}

cl_int RetainProgram(Program *program)
{
    return 0;
}

cl_int ReleaseProgram(Program *program)
{
    return 0;
}

cl_int BuildProgram(Program *program,
                    cl_uint num_devices,
                    Device *const *device_list,
                    const char *options,
                    void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
                    void *user_data)
{
    return 0;
}

cl_int CompileProgram(Program *program,
                      cl_uint num_devices,
                      Device *const *device_list,
                      const char *options,
                      cl_uint num_input_headers,
                      Program *const *input_headers,
                      const char **header_include_names,
                      void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
                      void *user_data)
{
    return 0;
}

Program *LinkProgram(Context *context,
                     cl_uint num_devices,
                     Device *const *device_list,
                     const char *options,
                     cl_uint num_input_programs,
                     Program *const *input_programs,
                     void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
                     void *user_data,
                     cl_int *errcode_ret)
{
    return 0;
}

cl_int SetProgramReleaseCallback(Program *program,
                                 void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
                                 void *user_data)
{
    return 0;
}

cl_int SetProgramSpecializationConstant(Program *program,
                                        cl_uint spec_id,
                                        size_t spec_size,
                                        const void *spec_value)
{
    return 0;
}

cl_int UnloadPlatformCompiler(Platform *platform)
{
    return 0;
}

cl_int GetProgramInfo(Program *program,
                      ProgramInfo param_name,
                      size_t param_value_size,
                      void *param_value,
                      size_t *param_value_size_ret)
{
    return 0;
}

cl_int GetProgramBuildInfo(Program *program,
                           Device *device,
                           ProgramBuildInfo param_name,
                           size_t param_value_size,
                           void *param_value,
                           size_t *param_value_size_ret)
{
    return 0;
}

Kernel *CreateKernel(Program *program, const char *kernel_name, cl_int *errcode_ret)
{
    return 0;
}

cl_int CreateKernelsInProgram(Program *program,
                              cl_uint num_kernels,
                              Kernel **kernels,
                              cl_uint *num_kernels_ret)
{
    return 0;
}

Kernel *CloneKernel(Kernel *source_kernel, cl_int *errcode_ret)
{
    return 0;
}

cl_int RetainKernel(Kernel *kernel)
{
    return 0;
}

cl_int ReleaseKernel(Kernel *kernel)
{
    return 0;
}

cl_int SetKernelArg(Kernel *kernel, cl_uint arg_index, size_t arg_size, const void *arg_value)
{
    return 0;
}

cl_int SetKernelArgSVMPointer(Kernel *kernel, cl_uint arg_index, const void *arg_value)
{
    return 0;
}

cl_int SetKernelExecInfo(Kernel *kernel,
                         KernelExecInfo param_name,
                         size_t param_value_size,
                         const void *param_value)
{
    return 0;
}

cl_int GetKernelInfo(Kernel *kernel,
                     KernelInfo param_name,
                     size_t param_value_size,
                     void *param_value,
                     size_t *param_value_size_ret)
{
    return 0;
}

cl_int GetKernelArgInfo(Kernel *kernel,
                        cl_uint arg_index,
                        KernelArgInfo param_name,
                        size_t param_value_size,
                        void *param_value,
                        size_t *param_value_size_ret)
{
    return 0;
}

cl_int GetKernelWorkGroupInfo(Kernel *kernel,
                              Device *device,
                              KernelWorkGroupInfo param_name,
                              size_t param_value_size,
                              void *param_value,
                              size_t *param_value_size_ret)
{
    return 0;
}

cl_int GetKernelSubGroupInfo(Kernel *kernel,
                             Device *device,
                             KernelSubGroupInfo param_name,
                             size_t input_value_size,
                             const void *input_value,
                             size_t param_value_size,
                             void *param_value,
                             size_t *param_value_size_ret)
{
    return 0;
}

cl_int WaitForEvents(cl_uint num_events, Event *const *event_list)
{
    return 0;
}

cl_int GetEventInfo(Event *event,
                    EventInfo param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret)
{
    return 0;
}

Event *CreateUserEvent(Context *context, cl_int *errcode_ret)
{
    return 0;
}

cl_int RetainEvent(Event *event)
{
    return 0;
}

cl_int ReleaseEvent(Event *event)
{
    return 0;
}

cl_int SetUserEventStatus(Event *event, cl_int execution_status)
{
    return 0;
}

cl_int SetEventCallback(Event *event,
                        cl_int command_exec_callback_type,
                        void(CL_CALLBACK *pfn_notify)(cl_event event,
                                                      cl_int event_command_status,
                                                      void *user_data),
                        void *user_data)
{
    return 0;
}

cl_int GetEventProfilingInfo(Event *event,
                             ProfilingInfo param_name,
                             size_t param_value_size,
                             void *param_value,
                             size_t *param_value_size_ret)
{
    return 0;
}

cl_int Flush(CommandQueue *command_queue)
{
    return 0;
}

cl_int Finish(CommandQueue *command_queue)
{
    return 0;
}

cl_int EnqueueReadBuffer(CommandQueue *command_queue,
                         Memory *buffer,
                         cl_bool blocking_read,
                         size_t offset,
                         size_t size,
                         void *ptr,
                         cl_uint num_events_in_wait_list,
                         Event *const *event_wait_list,
                         Event **event)
{
    return 0;
}

cl_int EnqueueReadBufferRect(CommandQueue *command_queue,
                             Memory *buffer,
                             cl_bool blocking_read,
                             const size_t *buffer_origin,
                             const size_t *host_origin,
                             const size_t *region,
                             size_t buffer_row_pitch,
                             size_t buffer_slice_pitch,
                             size_t host_row_pitch,
                             size_t host_slice_pitch,
                             void *ptr,
                             cl_uint num_events_in_wait_list,
                             Event *const *event_wait_list,
                             Event **event)
{
    return 0;
}

cl_int EnqueueWriteBuffer(CommandQueue *command_queue,
                          Memory *buffer,
                          cl_bool blocking_write,
                          size_t offset,
                          size_t size,
                          const void *ptr,
                          cl_uint num_events_in_wait_list,
                          Event *const *event_wait_list,
                          Event **event)
{
    return 0;
}

cl_int EnqueueWriteBufferRect(CommandQueue *command_queue,
                              Memory *buffer,
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
                              Event *const *event_wait_list,
                              Event **event)
{
    return 0;
}

cl_int EnqueueFillBuffer(CommandQueue *command_queue,
                         Memory *buffer,
                         const void *pattern,
                         size_t pattern_size,
                         size_t offset,
                         size_t size,
                         cl_uint num_events_in_wait_list,
                         Event *const *event_wait_list,
                         Event **event)
{
    return 0;
}

cl_int EnqueueCopyBuffer(CommandQueue *command_queue,
                         Memory *src_buffer,
                         Memory *dst_buffer,
                         size_t src_offset,
                         size_t dst_offset,
                         size_t size,
                         cl_uint num_events_in_wait_list,
                         Event *const *event_wait_list,
                         Event **event)
{
    return 0;
}

cl_int EnqueueCopyBufferRect(CommandQueue *command_queue,
                             Memory *src_buffer,
                             Memory *dst_buffer,
                             const size_t *src_origin,
                             const size_t *dst_origin,
                             const size_t *region,
                             size_t src_row_pitch,
                             size_t src_slice_pitch,
                             size_t dst_row_pitch,
                             size_t dst_slice_pitch,
                             cl_uint num_events_in_wait_list,
                             Event *const *event_wait_list,
                             Event **event)
{
    return 0;
}

cl_int EnqueueReadImage(CommandQueue *command_queue,
                        Memory *image,
                        cl_bool blocking_read,
                        const size_t *origin,
                        const size_t *region,
                        size_t row_pitch,
                        size_t slice_pitch,
                        void *ptr,
                        cl_uint num_events_in_wait_list,
                        Event *const *event_wait_list,
                        Event **event)
{
    return 0;
}

cl_int EnqueueWriteImage(CommandQueue *command_queue,
                         Memory *image,
                         cl_bool blocking_write,
                         const size_t *origin,
                         const size_t *region,
                         size_t input_row_pitch,
                         size_t input_slice_pitch,
                         const void *ptr,
                         cl_uint num_events_in_wait_list,
                         Event *const *event_wait_list,
                         Event **event)
{
    return 0;
}

cl_int EnqueueFillImage(CommandQueue *command_queue,
                        Memory *image,
                        const void *fill_color,
                        const size_t *origin,
                        const size_t *region,
                        cl_uint num_events_in_wait_list,
                        Event *const *event_wait_list,
                        Event **event)
{
    return 0;
}

cl_int EnqueueCopyImage(CommandQueue *command_queue,
                        Memory *src_image,
                        Memory *dst_image,
                        const size_t *src_origin,
                        const size_t *dst_origin,
                        const size_t *region,
                        cl_uint num_events_in_wait_list,
                        Event *const *event_wait_list,
                        Event **event)
{
    return 0;
}

cl_int EnqueueCopyImageToBuffer(CommandQueue *command_queue,
                                Memory *src_image,
                                Memory *dst_buffer,
                                const size_t *src_origin,
                                const size_t *region,
                                size_t dst_offset,
                                cl_uint num_events_in_wait_list,
                                Event *const *event_wait_list,
                                Event **event)
{
    return 0;
}

cl_int EnqueueCopyBufferToImage(CommandQueue *command_queue,
                                Memory *src_buffer,
                                Memory *dst_image,
                                size_t src_offset,
                                const size_t *dst_origin,
                                const size_t *region,
                                cl_uint num_events_in_wait_list,
                                Event *const *event_wait_list,
                                Event **event)
{
    return 0;
}

void *EnqueueMapBuffer(CommandQueue *command_queue,
                       Memory *buffer,
                       cl_bool blocking_map,
                       cl_map_flags map_flags,
                       size_t offset,
                       size_t size,
                       cl_uint num_events_in_wait_list,
                       Event *const *event_wait_list,
                       Event **event,
                       cl_int *errcode_ret)
{
    return 0;
}

void *EnqueueMapImage(CommandQueue *command_queue,
                      Memory *image,
                      cl_bool blocking_map,
                      cl_map_flags map_flags,
                      const size_t *origin,
                      const size_t *region,
                      size_t *image_row_pitch,
                      size_t *image_slice_pitch,
                      cl_uint num_events_in_wait_list,
                      Event *const *event_wait_list,
                      Event **event,
                      cl_int *errcode_ret)
{
    return 0;
}

cl_int EnqueueUnmapMemObject(CommandQueue *command_queue,
                             Memory *memobj,
                             void *mapped_ptr,
                             cl_uint num_events_in_wait_list,
                             Event *const *event_wait_list,
                             Event **event)
{
    return 0;
}

cl_int EnqueueMigrateMemObjects(CommandQueue *command_queue,
                                cl_uint num_mem_objects,
                                Memory *const *mem_objects,
                                cl_mem_migration_flags flags,
                                cl_uint num_events_in_wait_list,
                                Event *const *event_wait_list,
                                Event **event)
{
    return 0;
}

cl_int EnqueueNDRangeKernel(CommandQueue *command_queue,
                            Kernel *kernel,
                            cl_uint work_dim,
                            const size_t *global_work_offset,
                            const size_t *global_work_size,
                            const size_t *local_work_size,
                            cl_uint num_events_in_wait_list,
                            Event *const *event_wait_list,
                            Event **event)
{
    return 0;
}

cl_int EnqueueNativeKernel(CommandQueue *command_queue,
                           void(CL_CALLBACK *user_func)(void *),
                           void *args,
                           size_t cb_args,
                           cl_uint num_mem_objects,
                           Memory *const *mem_list,
                           const void **args_mem_loc,
                           cl_uint num_events_in_wait_list,
                           Event *const *event_wait_list,
                           Event **event)
{
    return 0;
}

cl_int EnqueueMarkerWithWaitList(CommandQueue *command_queue,
                                 cl_uint num_events_in_wait_list,
                                 Event *const *event_wait_list,
                                 Event **event)
{
    return 0;
}

cl_int EnqueueBarrierWithWaitList(CommandQueue *command_queue,
                                  cl_uint num_events_in_wait_list,
                                  Event *const *event_wait_list,
                                  Event **event)
{
    return 0;
}

cl_int EnqueueSVMFree(CommandQueue *command_queue,
                      cl_uint num_svm_pointers,
                      void *svm_pointers[],
                      void(CL_CALLBACK *pfn_free_func)(cl_command_queue queue,
                                                       cl_uint num_svm_pointers,
                                                       void *svm_pointers[],
                                                       void *user_data),
                      void *user_data,
                      cl_uint num_events_in_wait_list,
                      Event *const *event_wait_list,
                      Event **event)
{
    return 0;
}

cl_int EnqueueSVMMemcpy(CommandQueue *command_queue,
                        cl_bool blocking_copy,
                        void *dst_ptr,
                        const void *src_ptr,
                        size_t size,
                        cl_uint num_events_in_wait_list,
                        Event *const *event_wait_list,
                        Event **event)
{
    return 0;
}

cl_int EnqueueSVMMemFill(CommandQueue *command_queue,
                         void *svm_ptr,
                         const void *pattern,
                         size_t pattern_size,
                         size_t size,
                         cl_uint num_events_in_wait_list,
                         Event *const *event_wait_list,
                         Event **event)
{
    return 0;
}

cl_int EnqueueSVMMap(CommandQueue *command_queue,
                     cl_bool blocking_map,
                     cl_map_flags flags,
                     void *svm_ptr,
                     size_t size,
                     cl_uint num_events_in_wait_list,
                     Event *const *event_wait_list,
                     Event **event)
{
    return 0;
}

cl_int EnqueueSVMUnmap(CommandQueue *command_queue,
                       void *svm_ptr,
                       cl_uint num_events_in_wait_list,
                       Event *const *event_wait_list,
                       Event **event)
{
    return 0;
}

cl_int EnqueueSVMMigrateMem(CommandQueue *command_queue,
                            cl_uint num_svm_pointers,
                            const void **svm_pointers,
                            const size_t *sizes,
                            cl_mem_migration_flags flags,
                            cl_uint num_events_in_wait_list,
                            Event *const *event_wait_list,
                            Event **event)
{
    return 0;
}

void *GetExtensionFunctionAddressForPlatform(Platform *platform, const char *func_name)
{
    return 0;
}

cl_int SetCommandQueueProperty(CommandQueue *command_queue,
                               cl_command_queue_properties properties,
                               cl_bool enable,
                               cl_command_queue_properties *old_properties)
{
    return 0;
}

Memory *CreateImage2D(Context *context,
                      cl_mem_flags flags,
                      const cl_image_format *image_format,
                      size_t image_width,
                      size_t image_height,
                      size_t image_row_pitch,
                      void *host_ptr,
                      cl_int *errcode_ret)
{
    return 0;
}

Memory *CreateImage3D(Context *context,
                      cl_mem_flags flags,
                      const cl_image_format *image_format,
                      size_t image_width,
                      size_t image_height,
                      size_t image_depth,
                      size_t image_row_pitch,
                      size_t image_slice_pitch,
                      void *host_ptr,
                      cl_int *errcode_ret)
{
    return 0;
}

cl_int EnqueueMarker(CommandQueue *command_queue, Event **event)
{
    return 0;
}

cl_int EnqueueWaitForEvents(CommandQueue *command_queue,
                            cl_uint num_events,
                            Event *const *event_list)
{
    return 0;
}

cl_int EnqueueBarrier(CommandQueue *command_queue)
{
    return 0;
}

cl_int UnloadCompiler()
{
    return 0;
}

void *GetExtensionFunctionAddress(const char *func_name)
{
    return 0;
}

CommandQueue *CreateCommandQueue(Context *context,
                                 Device *device,
                                 cl_command_queue_properties properties,
                                 cl_int *errcode_ret)
{
    return 0;
}

Sampler *CreateSampler(Context *context,
                       cl_bool normalized_coords,
                       AddressingMode addressing_mode,
                       FilterMode filter_mode,
                       cl_int *errcode_ret)
{
    return 0;
}

cl_int EnqueueTask(CommandQueue *command_queue,
                   Kernel *kernel,
                   cl_uint num_events_in_wait_list,
                   Event *const *event_wait_list,
                   Event **event)
{
    return 0;
}
}  // namespace cl
