//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// cl_stubs.cpp: Stubs for CL entry points.

#include "libGLESv2/cl_stubs_autogen.h"

#include "libANGLE/CLPlatform.h"
#include "libGLESv2/cl_dispatch_table.h"
#include "libGLESv2/proc_table_cl.h"

#ifdef ANGLE_ENABLE_CL_PASSTHROUGH
#    include "libANGLE/renderer/cl/CLPlatformCL.h"
#endif
#ifdef ANGLE_ENABLE_VULKAN
#    include "libANGLE/renderer/vulkan/CLPlatformVk.h"
#endif

#include "libANGLE/Debug.h"

#include <cstring>

#define WARN_NOT_SUPPORTED(command)                                         \
    do                                                                      \
    {                                                                       \
        static bool sWarned = false;                                        \
        if (!sWarned)                                                       \
        {                                                                   \
            sWarned = true;                                                 \
            WARN() << "OpenCL command " #command " is not (yet) supported"; \
        }                                                                   \
    } while (0)

namespace cl
{

namespace
{

const Platform::List &InitializePlatforms(bool isIcd)
{
    static bool initialized = false;
    if (!initialized)
    {
        initialized = true;

#ifdef ANGLE_ENABLE_CL_PASSTHROUGH
        rx::CLPlatformImpl::ImplList implListCL = rx::CLPlatformCL::GetPlatforms(isIcd);
        while (!implListCL.empty())
        {
            Platform::CreatePlatform(gCLIcdDispatchTable, std::move(implListCL.front()));
            implListCL.pop_front();
        }
#endif

#ifdef ANGLE_ENABLE_VULKAN
        rx::CLPlatformImpl::ImplList implListVk = rx::CLPlatformVk::GetPlatforms();
        while (!implListVk.empty())
        {
            Platform::CreatePlatform(gCLIcdDispatchTable, std::move(implListVk.front()));
            implListVk.pop_front();
        }
#endif
    }
    return Platform::GetPlatforms();
}

cl_int GetPlatforms(cl_uint num_entries, Platform **platforms, cl_uint *num_platforms, bool isIcd)
{
    const Platform::List &platformList = InitializePlatforms(isIcd);
    if (num_platforms != nullptr)
    {
        *num_platforms = static_cast<cl_uint>(platformList.size());
    }
    if (platforms != nullptr)
    {
        cl_uint entry   = 0u;
        auto platformIt = platformList.cbegin();
        while (entry < num_entries && platformIt != platformList.cend())
        {
            platforms[entry++] = (*platformIt++).get();
        }
    }
    return CL_SUCCESS;
}

}  // anonymous namespace

cl_int IcdGetPlatformIDsKHR(cl_uint num_entries, Platform **platforms, cl_uint *num_platforms)
{
    return GetPlatforms(num_entries, platforms, num_platforms, true);
}

cl_int GetPlatformIDs(cl_uint num_entries, Platform **platforms, cl_uint *num_platforms)
{
    return GetPlatforms(num_entries, platforms, num_platforms, false);
}

cl_int GetPlatformInfo(Platform *platform,
                       PlatformInfo param_name,
                       size_t param_value_size,
                       void *param_value,
                       size_t *param_value_size_ret)
{
    cl_version version    = 0u;
    cl_ulong hostTimerRes = 0u;
    const void *value     = nullptr;
    size_t value_size     = 0u;
    switch (param_name)
    {
        case PlatformInfo::Profile:
            value      = platform->getProfile();
            value_size = std::strlen(platform->getProfile()) + 1u;
            break;
        case PlatformInfo::Version:
            value      = platform->getVersionString();
            value_size = std::strlen(platform->getVersionString()) + 1u;
            break;
        case PlatformInfo::NumericVersion:
            version    = platform->getVersion();
            value      = &version;
            value_size = sizeof(version);
            break;
        case PlatformInfo::Name:
            value      = platform->getName();
            value_size = std::strlen(platform->getName()) + 1u;
            break;
        case PlatformInfo::Vendor:
            value      = Platform::GetVendor();
            value_size = std::strlen(Platform::GetVendor()) + 1u;
            break;
        case PlatformInfo::Extensions:
            value      = platform->getExtensions();
            value_size = std::strlen(platform->getExtensions()) + 1u;
            break;
        case PlatformInfo::ExtensionsWithVersion:
            if (platform->getExtensionsWithVersion().empty())
            {
                return CL_INVALID_VALUE;
            }
            value      = platform->getExtensionsWithVersion().data();
            value_size = platform->getExtensionsWithVersion().size() * sizeof(cl_name_version);
            break;
        case PlatformInfo::HostTimerResolution:
            hostTimerRes = platform->getHostTimerResolution();
            value        = &hostTimerRes;
            value_size   = sizeof(hostTimerRes);
            break;
        case PlatformInfo::IcdSuffix:
            value      = Platform::GetIcdSuffix();
            value_size = std::strlen(Platform::GetIcdSuffix()) + 1u;
            break;
        default:
            return CL_INVALID_VALUE;
    }
    if (param_value != nullptr)
    {
        if (param_value_size < value_size)
        {
            return CL_INVALID_VALUE;
        }
        if (value != nullptr)
        {
            std::memcpy(param_value, value, value_size);
        }
    }
    if (param_value_size_ret != nullptr)
    {
        *param_value_size_ret = value_size;
    }
    return CL_SUCCESS;
}

cl_int GetDeviceIDs(Platform *platform,
                    cl_device_type device_type,
                    cl_uint num_entries,
                    Device **devices,
                    cl_uint *num_devices)
{
    WARN_NOT_SUPPORTED(GetDeviceIDs);
    return 0;
}

cl_int GetDeviceInfo(Device *device,
                     DeviceInfo param_name,
                     size_t param_value_size,
                     void *param_value,
                     size_t *param_value_size_ret)
{
    WARN_NOT_SUPPORTED(GetDeviceInfo);
    return 0;
}

cl_int CreateSubDevices(Device *in_device,
                        const cl_device_partition_property *properties,
                        cl_uint num_devices,
                        Device **out_devices,
                        cl_uint *num_devices_ret)
{
    WARN_NOT_SUPPORTED(CreateSubDevices);
    return 0;
}

cl_int RetainDevice(Device *device)
{
    WARN_NOT_SUPPORTED(RetainDevice);
    return 0;
}

cl_int ReleaseDevice(Device *device)
{
    WARN_NOT_SUPPORTED(ReleaseDevice);
    return 0;
}

cl_int SetDefaultDeviceCommandQueue(Context *context, Device *device, CommandQueue *command_queue)
{
    WARN_NOT_SUPPORTED(SetDefaultDeviceCommandQueue);
    return 0;
}

cl_int GetDeviceAndHostTimer(Device *device, cl_ulong *device_timestamp, cl_ulong *host_timestamp)
{
    WARN_NOT_SUPPORTED(GetDeviceAndHostTimer);
    return 0;
}

cl_int GetHostTimer(Device *device, cl_ulong *host_timestamp)
{
    WARN_NOT_SUPPORTED(GetHostTimer);
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
    WARN_NOT_SUPPORTED(CreateContext);
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
    WARN_NOT_SUPPORTED(CreateContextFromType);
    return 0;
}

cl_int RetainContext(Context *context)
{
    WARN_NOT_SUPPORTED(RetainContext);
    return 0;
}

cl_int ReleaseContext(Context *context)
{
    WARN_NOT_SUPPORTED(ReleaseContext);
    return 0;
}

cl_int GetContextInfo(Context *context,
                      ContextInfo param_name,
                      size_t param_value_size,
                      void *param_value,
                      size_t *param_value_size_ret)
{
    WARN_NOT_SUPPORTED(GetContextInfo);
    return 0;
}

cl_int SetContextDestructorCallback(Context *context,
                                    void(CL_CALLBACK *pfn_notify)(cl_context context,
                                                                  void *user_data),
                                    void *user_data)
{
    WARN_NOT_SUPPORTED(SetContextDestructorCallback);
    return 0;
}

CommandQueue *CreateCommandQueueWithProperties(Context *context,
                                               Device *device,
                                               const cl_queue_properties *properties,
                                               cl_int *errcode_ret)
{
    WARN_NOT_SUPPORTED(CreateCommandQueueWithProperties);
    return 0;
}

cl_int RetainCommandQueue(CommandQueue *command_queue)
{
    WARN_NOT_SUPPORTED(RetainCommandQueue);
    return 0;
}

cl_int ReleaseCommandQueue(CommandQueue *command_queue)
{
    WARN_NOT_SUPPORTED(ReleaseCommandQueue);
    return 0;
}

cl_int GetCommandQueueInfo(CommandQueue *command_queue,
                           CommandQueueInfo param_name,
                           size_t param_value_size,
                           void *param_value,
                           size_t *param_value_size_ret)
{
    WARN_NOT_SUPPORTED(GetCommandQueueInfo);
    return 0;
}

Memory *CreateBuffer(Context *context,
                     cl_mem_flags flags,
                     size_t size,
                     void *host_ptr,
                     cl_int *errcode_ret)
{
    WARN_NOT_SUPPORTED(CreateBuffer);
    return 0;
}

Memory *CreateBufferWithProperties(Context *context,
                                   const cl_mem_properties *properties,
                                   cl_mem_flags flags,
                                   size_t size,
                                   void *host_ptr,
                                   cl_int *errcode_ret)
{
    WARN_NOT_SUPPORTED(CreateBufferWithProperties);
    return 0;
}

Memory *CreateSubBuffer(Memory *buffer,
                        cl_mem_flags flags,
                        cl_buffer_create_type buffer_create_type,
                        const void *buffer_create_info,
                        cl_int *errcode_ret)
{
    WARN_NOT_SUPPORTED(CreateSubBuffer);
    return 0;
}

Memory *CreateImage(Context *context,
                    cl_mem_flags flags,
                    const cl_image_format *image_format,
                    const cl_image_desc *image_desc,
                    void *host_ptr,
                    cl_int *errcode_ret)
{
    WARN_NOT_SUPPORTED(CreateImage);
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
    WARN_NOT_SUPPORTED(CreateImageWithProperties);
    return 0;
}

Memory *CreatePipe(Context *context,
                   cl_mem_flags flags,
                   cl_uint pipe_packet_size,
                   cl_uint pipe_max_packets,
                   const cl_pipe_properties *properties,
                   cl_int *errcode_ret)
{
    WARN_NOT_SUPPORTED(CreatePipe);
    return 0;
}

cl_int RetainMemObject(Memory *memobj)
{
    WARN_NOT_SUPPORTED(RetainMemObject);
    return 0;
}

cl_int ReleaseMemObject(Memory *memobj)
{
    WARN_NOT_SUPPORTED(ReleaseMemObject);
    return 0;
}

cl_int GetSupportedImageFormats(Context *context,
                                cl_mem_flags flags,
                                MemObjectType image_type,
                                cl_uint num_entries,
                                cl_image_format *image_formats,
                                cl_uint *num_image_formats)
{
    WARN_NOT_SUPPORTED(GetSupportedImageFormats);
    return 0;
}

cl_int GetMemObjectInfo(Memory *memobj,
                        MemInfo param_name,
                        size_t param_value_size,
                        void *param_value,
                        size_t *param_value_size_ret)
{
    WARN_NOT_SUPPORTED(GetMemObjectInfo);
    return 0;
}

cl_int GetImageInfo(Memory *image,
                    ImageInfo param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret)
{
    WARN_NOT_SUPPORTED(GetImageInfo);
    return 0;
}

cl_int GetPipeInfo(Memory *pipe,
                   PipeInfo param_name,
                   size_t param_value_size,
                   void *param_value,
                   size_t *param_value_size_ret)
{
    WARN_NOT_SUPPORTED(GetPipeInfo);
    return 0;
}

cl_int SetMemObjectDestructorCallback(Memory *memobj,
                                      void(CL_CALLBACK *pfn_notify)(cl_mem memobj, void *user_data),
                                      void *user_data)
{
    WARN_NOT_SUPPORTED(SetMemObjectDestructorCallback);
    return 0;
}

void *SVMAlloc(Context *context, cl_svm_mem_flags flags, size_t size, cl_uint alignment)
{
    WARN_NOT_SUPPORTED(SVMAlloc);
    return 0;
}

void SVMFree(Context *context, void *svm_pointer)
{
    WARN_NOT_SUPPORTED(SVMFree);
}

Sampler *CreateSamplerWithProperties(Context *context,
                                     const cl_sampler_properties *sampler_properties,
                                     cl_int *errcode_ret)
{
    WARN_NOT_SUPPORTED(CreateSamplerWithProperties);
    return 0;
}

cl_int RetainSampler(Sampler *sampler)
{
    WARN_NOT_SUPPORTED(RetainSampler);
    return 0;
}

cl_int ReleaseSampler(Sampler *sampler)
{
    WARN_NOT_SUPPORTED(ReleaseSampler);
    return 0;
}

cl_int GetSamplerInfo(Sampler *sampler,
                      SamplerInfo param_name,
                      size_t param_value_size,
                      void *param_value,
                      size_t *param_value_size_ret)
{
    WARN_NOT_SUPPORTED(GetSamplerInfo);
    return 0;
}

Program *CreateProgramWithSource(Context *context,
                                 cl_uint count,
                                 const char **strings,
                                 const size_t *lengths,
                                 cl_int *errcode_ret)
{
    WARN_NOT_SUPPORTED(CreateProgramWithSource);
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
    WARN_NOT_SUPPORTED(CreateProgramWithBinary);
    return 0;
}

Program *CreateProgramWithBuiltInKernels(Context *context,
                                         cl_uint num_devices,
                                         Device *const *device_list,
                                         const char *kernel_names,
                                         cl_int *errcode_ret)
{
    WARN_NOT_SUPPORTED(CreateProgramWithBuiltInKernels);
    return 0;
}

Program *CreateProgramWithIL(Context *context, const void *il, size_t length, cl_int *errcode_ret)
{
    WARN_NOT_SUPPORTED(CreateProgramWithIL);
    return 0;
}

cl_int RetainProgram(Program *program)
{
    WARN_NOT_SUPPORTED(RetainProgram);
    return 0;
}

cl_int ReleaseProgram(Program *program)
{
    WARN_NOT_SUPPORTED(ReleaseProgram);
    return 0;
}

cl_int BuildProgram(Program *program,
                    cl_uint num_devices,
                    Device *const *device_list,
                    const char *options,
                    void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
                    void *user_data)
{
    WARN_NOT_SUPPORTED(BuildProgram);
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
    WARN_NOT_SUPPORTED(CompileProgram);
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
    WARN_NOT_SUPPORTED(LinkProgram);
    return 0;
}

cl_int SetProgramReleaseCallback(Program *program,
                                 void(CL_CALLBACK *pfn_notify)(cl_program program, void *user_data),
                                 void *user_data)
{
    WARN_NOT_SUPPORTED(SetProgramReleaseCallback);
    return 0;
}

cl_int SetProgramSpecializationConstant(Program *program,
                                        cl_uint spec_id,
                                        size_t spec_size,
                                        const void *spec_value)
{
    WARN_NOT_SUPPORTED(SetProgramSpecializationConstant);
    return 0;
}

cl_int UnloadPlatformCompiler(Platform *platform)
{
    WARN_NOT_SUPPORTED(UnloadPlatformCompiler);
    return 0;
}

cl_int GetProgramInfo(Program *program,
                      ProgramInfo param_name,
                      size_t param_value_size,
                      void *param_value,
                      size_t *param_value_size_ret)
{
    WARN_NOT_SUPPORTED(GetProgramInfo);
    return 0;
}

cl_int GetProgramBuildInfo(Program *program,
                           Device *device,
                           ProgramBuildInfo param_name,
                           size_t param_value_size,
                           void *param_value,
                           size_t *param_value_size_ret)
{
    WARN_NOT_SUPPORTED(GetProgramBuildInfo);
    return 0;
}

Kernel *CreateKernel(Program *program, const char *kernel_name, cl_int *errcode_ret)
{
    WARN_NOT_SUPPORTED(CreateKernel);
    return 0;
}

cl_int CreateKernelsInProgram(Program *program,
                              cl_uint num_kernels,
                              Kernel **kernels,
                              cl_uint *num_kernels_ret)
{
    WARN_NOT_SUPPORTED(CreateKernelsInProgram);
    return 0;
}

Kernel *CloneKernel(Kernel *source_kernel, cl_int *errcode_ret)
{
    WARN_NOT_SUPPORTED(CloneKernel);
    return 0;
}

cl_int RetainKernel(Kernel *kernel)
{
    WARN_NOT_SUPPORTED(RetainKernel);
    return 0;
}

cl_int ReleaseKernel(Kernel *kernel)
{
    WARN_NOT_SUPPORTED(ReleaseKernel);
    return 0;
}

cl_int SetKernelArg(Kernel *kernel, cl_uint arg_index, size_t arg_size, const void *arg_value)
{
    WARN_NOT_SUPPORTED(SetKernelArg);
    return 0;
}

cl_int SetKernelArgSVMPointer(Kernel *kernel, cl_uint arg_index, const void *arg_value)
{
    WARN_NOT_SUPPORTED(SetKernelArgSVMPointer);
    return 0;
}

cl_int SetKernelExecInfo(Kernel *kernel,
                         KernelExecInfo param_name,
                         size_t param_value_size,
                         const void *param_value)
{
    WARN_NOT_SUPPORTED(SetKernelExecInfo);
    return 0;
}

cl_int GetKernelInfo(Kernel *kernel,
                     KernelInfo param_name,
                     size_t param_value_size,
                     void *param_value,
                     size_t *param_value_size_ret)
{
    WARN_NOT_SUPPORTED(GetKernelInfo);
    return 0;
}

cl_int GetKernelArgInfo(Kernel *kernel,
                        cl_uint arg_index,
                        KernelArgInfo param_name,
                        size_t param_value_size,
                        void *param_value,
                        size_t *param_value_size_ret)
{
    WARN_NOT_SUPPORTED(GetKernelArgInfo);
    return 0;
}

cl_int GetKernelWorkGroupInfo(Kernel *kernel,
                              Device *device,
                              KernelWorkGroupInfo param_name,
                              size_t param_value_size,
                              void *param_value,
                              size_t *param_value_size_ret)
{
    WARN_NOT_SUPPORTED(GetKernelWorkGroupInfo);
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
    WARN_NOT_SUPPORTED(GetKernelSubGroupInfo);
    return 0;
}

cl_int WaitForEvents(cl_uint num_events, Event *const *event_list)
{
    WARN_NOT_SUPPORTED(WaitForEvents);
    return 0;
}

cl_int GetEventInfo(Event *event,
                    EventInfo param_name,
                    size_t param_value_size,
                    void *param_value,
                    size_t *param_value_size_ret)
{
    WARN_NOT_SUPPORTED(GetEventInfo);
    return 0;
}

Event *CreateUserEvent(Context *context, cl_int *errcode_ret)
{
    WARN_NOT_SUPPORTED(CreateUserEvent);
    return 0;
}

cl_int RetainEvent(Event *event)
{
    WARN_NOT_SUPPORTED(RetainEvent);
    return 0;
}

cl_int ReleaseEvent(Event *event)
{
    WARN_NOT_SUPPORTED(ReleaseEvent);
    return 0;
}

cl_int SetUserEventStatus(Event *event, cl_int execution_status)
{
    WARN_NOT_SUPPORTED(SetUserEventStatus);
    return 0;
}

cl_int SetEventCallback(Event *event,
                        cl_int command_exec_callback_type,
                        void(CL_CALLBACK *pfn_notify)(cl_event event,
                                                      cl_int event_command_status,
                                                      void *user_data),
                        void *user_data)
{
    WARN_NOT_SUPPORTED(SetEventCallback);
    return 0;
}

cl_int GetEventProfilingInfo(Event *event,
                             ProfilingInfo param_name,
                             size_t param_value_size,
                             void *param_value,
                             size_t *param_value_size_ret)
{
    WARN_NOT_SUPPORTED(GetEventProfilingInfo);
    return 0;
}

cl_int Flush(CommandQueue *command_queue)
{
    WARN_NOT_SUPPORTED(Flush);
    return 0;
}

cl_int Finish(CommandQueue *command_queue)
{
    WARN_NOT_SUPPORTED(Finish);
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
    WARN_NOT_SUPPORTED(EnqueueReadBuffer);
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
    WARN_NOT_SUPPORTED(EnqueueReadBufferRect);
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
    WARN_NOT_SUPPORTED(EnqueueWriteBuffer);
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
    WARN_NOT_SUPPORTED(EnqueueWriteBufferRect);
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
    WARN_NOT_SUPPORTED(EnqueueFillBuffer);
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
    WARN_NOT_SUPPORTED(EnqueueCopyBuffer);
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
    WARN_NOT_SUPPORTED(EnqueueCopyBufferRect);
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
    WARN_NOT_SUPPORTED(EnqueueReadImage);
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
    WARN_NOT_SUPPORTED(EnqueueWriteImage);
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
    WARN_NOT_SUPPORTED(EnqueueFillImage);
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
    WARN_NOT_SUPPORTED(EnqueueCopyImage);
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
    WARN_NOT_SUPPORTED(EnqueueCopyImageToBuffer);
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
    WARN_NOT_SUPPORTED(EnqueueCopyBufferToImage);
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
    WARN_NOT_SUPPORTED(EnqueueMapBuffer);
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
    WARN_NOT_SUPPORTED(EnqueueMapImage);
    return 0;
}

cl_int EnqueueUnmapMemObject(CommandQueue *command_queue,
                             Memory *memobj,
                             void *mapped_ptr,
                             cl_uint num_events_in_wait_list,
                             Event *const *event_wait_list,
                             Event **event)
{
    WARN_NOT_SUPPORTED(EnqueueUnmapMemObject);
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
    WARN_NOT_SUPPORTED(EnqueueMigrateMemObjects);
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
    WARN_NOT_SUPPORTED(EnqueueNDRangeKernel);
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
    WARN_NOT_SUPPORTED(EnqueueNativeKernel);
    return 0;
}

cl_int EnqueueMarkerWithWaitList(CommandQueue *command_queue,
                                 cl_uint num_events_in_wait_list,
                                 Event *const *event_wait_list,
                                 Event **event)
{
    WARN_NOT_SUPPORTED(EnqueueMarkerWithWaitList);
    return 0;
}

cl_int EnqueueBarrierWithWaitList(CommandQueue *command_queue,
                                  cl_uint num_events_in_wait_list,
                                  Event *const *event_wait_list,
                                  Event **event)
{
    WARN_NOT_SUPPORTED(EnqueueBarrierWithWaitList);
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
    WARN_NOT_SUPPORTED(EnqueueSVMFree);
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
    WARN_NOT_SUPPORTED(EnqueueSVMMemcpy);
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
    WARN_NOT_SUPPORTED(EnqueueSVMMemFill);
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
    WARN_NOT_SUPPORTED(EnqueueSVMMap);
    return 0;
}

cl_int EnqueueSVMUnmap(CommandQueue *command_queue,
                       void *svm_ptr,
                       cl_uint num_events_in_wait_list,
                       Event *const *event_wait_list,
                       Event **event)
{
    WARN_NOT_SUPPORTED(EnqueueSVMUnmap);
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
    WARN_NOT_SUPPORTED(EnqueueSVMMigrateMem);
    return 0;
}

void *GetExtensionFunctionAddressForPlatform(Platform *platform, const char *func_name)
{
    return GetExtensionFunctionAddress(func_name);
}

cl_int SetCommandQueueProperty(CommandQueue *command_queue,
                               cl_command_queue_properties properties,
                               cl_bool enable,
                               cl_command_queue_properties *old_properties)
{
    WARN_NOT_SUPPORTED(SetCommandQueueProperty);
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
    WARN_NOT_SUPPORTED(CreateImage2D);
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
    WARN_NOT_SUPPORTED(CreateImage3D);
    return 0;
}

cl_int EnqueueMarker(CommandQueue *command_queue, Event **event)
{
    WARN_NOT_SUPPORTED(EnqueueMarker);
    return 0;
}

cl_int EnqueueWaitForEvents(CommandQueue *command_queue,
                            cl_uint num_events,
                            Event *const *event_list)
{
    WARN_NOT_SUPPORTED(EnqueueWaitForEvents);
    return 0;
}

cl_int EnqueueBarrier(CommandQueue *command_queue)
{
    WARN_NOT_SUPPORTED(EnqueueBarrier);
    return 0;
}

cl_int UnloadCompiler()
{
    WARN_NOT_SUPPORTED(UnloadCompiler);
    return 0;
}

void *GetExtensionFunctionAddress(const char *func_name)
{
    if (func_name == nullptr)
    {
        return nullptr;
    }
    const ProcTable &procTable = GetProcTable();
    const auto it              = procTable.find(func_name);
    return it != procTable.cend() ? it->second : nullptr;
}

CommandQueue *CreateCommandQueue(Context *context,
                                 Device *device,
                                 cl_command_queue_properties properties,
                                 cl_int *errcode_ret)
{
    WARN_NOT_SUPPORTED(CreateCommandQueue);
    return 0;
}

Sampler *CreateSampler(Context *context,
                       cl_bool normalized_coords,
                       AddressingMode addressing_mode,
                       FilterMode filter_mode,
                       cl_int *errcode_ret)
{
    WARN_NOT_SUPPORTED(CreateSampler);
    return 0;
}

cl_int EnqueueTask(CommandQueue *command_queue,
                   Kernel *kernel,
                   cl_uint num_events_in_wait_list,
                   Event *const *event_wait_list,
                   Event **event)
{
    WARN_NOT_SUPPORTED(EnqueueTask);
    return 0;
}

}  // namespace cl
