//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLContext.cpp: Implements the cl::Context class.

#include "libANGLE/CLContext.h"

#include "libANGLE/CLBuffer.h"
#include "libANGLE/CLImage.h"
#include "libANGLE/CLPlatform.h"

#include <cstring>

namespace cl
{

Context::~Context() = default;

bool Context::release()
{
    const bool released = removeRef();
    if (released)
    {
        mPlatform.destroyContext(this);
    }
    return released;
}

cl_int Context::getInfo(ContextInfo name, size_t valueSize, void *value, size_t *valueSizeRet) const
{
    cl_uint numDevices    = 0u;
    const void *copyValue = nullptr;
    size_t copySize       = 0u;

    switch (name)
    {
        case ContextInfo::ReferenceCount:
            copyValue = getRefCountPtr();
            copySize  = sizeof(*getRefCountPtr());
            break;
        case ContextInfo::NumDevices:
            numDevices = static_cast<decltype(numDevices)>(mDevices.size());
            copyValue  = &numDevices;
            copySize   = sizeof(numDevices);
            break;
        case ContextInfo::Devices:
            static_assert(sizeof(decltype(mDevices)::value_type) == sizeof(Device *),
                          "DeviceRefList has wrong element size");
            copyValue = mDevices.data();
            copySize  = mDevices.size() * sizeof(decltype(mDevices)::value_type);
            break;
        case ContextInfo::Properties:
            copyValue = mProperties.data();
            copySize  = mProperties.size() * sizeof(decltype(mProperties)::value_type);
            break;
        default:
            return CL_INVALID_VALUE;
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

cl_command_queue Context::createCommandQueue(cl_device_id device,
                                             cl_command_queue_properties properties,
                                             cl_int *errcodeRet)
{
    return createCommandQueue(
        new CommandQueue(*this, *static_cast<Device *>(device), properties, errcodeRet),
        errcodeRet);
}

cl_command_queue Context::createCommandQueueWithProperties(cl_device_id device,
                                                           const cl_queue_properties *properties,
                                                           cl_int *errcodeRet)
{
    CommandQueue::PropArray propArray;
    cl_command_queue_properties props = 0u;
    cl_uint size                      = CommandQueue::kNoSize;
    if (properties != nullptr)
    {
        const cl_queue_properties *propIt = properties;
        while (*propIt != 0)
        {
            switch (*propIt++)
            {
                case CL_QUEUE_PROPERTIES:
                    props = *propIt++;
                    break;
                case CL_QUEUE_SIZE:
                    size = static_cast<decltype(size)>(*propIt++);
                    break;
            }
        }
        // Include the trailing zero
        ++propIt;
        propArray.reserve(propIt - properties);
        propArray.insert(propArray.cend(), properties, propIt);
    }
    return createCommandQueue(new CommandQueue(*this, *static_cast<Device *>(device),
                                               std::move(propArray), props, size, errcodeRet),
                              errcodeRet);
}

cl_mem Context::createBuffer(const cl_mem_properties *properties,
                             cl_mem_flags flags,
                             size_t size,
                             void *hostPtr,
                             cl_int *errcodeRet)
{
    return createMemory(new Buffer(*this, {}, flags, size, hostPtr, errcodeRet), errcodeRet);
}

cl_mem Context::createImage(const cl_mem_properties *properties,
                            cl_mem_flags flags,
                            const cl_image_format *format,
                            const cl_image_desc *desc,
                            void *hostPtr,
                            cl_int *errcodeRet)
{
    const ImageDescriptor imageDesc = {
        desc->image_type,        desc->image_width,      desc->image_height,
        desc->image_depth,       desc->image_array_size, desc->image_row_pitch,
        desc->image_slice_pitch, desc->num_mip_levels,   desc->num_samples};
    return createMemory(new Image(*this, {}, flags, *format, imageDesc,
                                  static_cast<Memory *>(desc->buffer), hostPtr, errcodeRet),
                        errcodeRet);
}

cl_mem Context::createImage2D(cl_mem_flags flags,
                              const cl_image_format *format,
                              size_t width,
                              size_t height,
                              size_t rowPitch,
                              void *hostPtr,
                              cl_int *errcodeRet)
{
    const ImageDescriptor imageDesc = {
        CL_MEM_OBJECT_IMAGE2D, width, height, 0u, 0u, rowPitch, 0u, 0u, 0u};
    return createMemory(
        new Image(*this, {}, flags, *format, imageDesc, nullptr, hostPtr, errcodeRet), errcodeRet);
}

cl_mem Context::createImage3D(cl_mem_flags flags,
                              const cl_image_format *format,
                              size_t width,
                              size_t height,
                              size_t depth,
                              size_t rowPitch,
                              size_t slicePitch,
                              void *hostPtr,
                              cl_int *errcodeRet)
{
    const ImageDescriptor imageDesc = {
        CL_MEM_OBJECT_IMAGE3D, width, height, depth, 0u, rowPitch, slicePitch, 0u, 0u};
    return createMemory(
        new Image(*this, {}, flags, *format, imageDesc, nullptr, hostPtr, errcodeRet), errcodeRet);
}

cl_sampler Context::createSampler(cl_bool normalizedCoords,
                                  AddressingMode addressingMode,
                                  FilterMode filterMode,
                                  cl_int *errcodeRet)
{
    return createSampler(
        new Sampler(*this, {}, normalizedCoords, addressingMode, filterMode, errcodeRet),
        errcodeRet);
}

cl_sampler Context::createSamplerWithProperties(const cl_sampler_properties *properties,
                                                cl_int *errcodeRet)
{
    Sampler::PropArray propArray;
    cl_bool normalizedCoords      = CL_TRUE;
    AddressingMode addressingMode = AddressingMode::Clamp;
    FilterMode filterMode         = FilterMode::Nearest;

    if (properties != nullptr)
    {
        const cl_sampler_properties *propIt = properties;
        while (*propIt != 0)
        {
            switch (*propIt++)
            {
                case CL_SAMPLER_NORMALIZED_COORDS:
                    normalizedCoords = static_cast<decltype(normalizedCoords)>(*propIt++);
                    break;
                case CL_SAMPLER_ADDRESSING_MODE:
                    addressingMode = FromCLenum<AddressingMode>(static_cast<CLenum>(*propIt++));
                    break;
                case CL_SAMPLER_FILTER_MODE:
                    filterMode = FromCLenum<FilterMode>(static_cast<CLenum>(*propIt++));
                    break;
            }
        }
        // Include the trailing zero
        ++propIt;
        propArray.reserve(propIt - properties);
        propArray.insert(propArray.cend(), properties, propIt);
    }

    return createSampler(new Sampler(*this, std::move(propArray), normalizedCoords, addressingMode,
                                     filterMode, errcodeRet),
                         errcodeRet);
}

cl_program Context::createProgramWithSource(cl_uint count,
                                            const char **strings,
                                            const size_t *lengths,
                                            cl_int *errcodeRet)
{
    std::string source;
    if (lengths == nullptr)
    {
        while (count-- != 0u)
        {
            source.append(*strings++);
        }
    }
    else
    {
        while (count-- != 0u)
        {
            if (*lengths != 0u)
            {
                source.append(*strings++, *lengths);
            }
            else
            {
                source.append(*strings++);
            }
            ++lengths;
        }
    }
    return createProgram(new Program(*this, std::move(source), errcodeRet), errcodeRet);
}

cl_program Context::createProgramWithIL(const void *il, size_t length, cl_int *errcodeRet)
{
    return createProgram(new Program(*this, il, length, errcodeRet), errcodeRet);
}

cl_program Context::createProgramWithBinary(cl_uint numDevices,
                                            const cl_device_id *devices,
                                            const size_t *lengths,
                                            const unsigned char **binaries,
                                            cl_int *binaryStatus,
                                            cl_int *errcodeRet)
{
    DeviceRefList refDevices;
    Binaries binaryVec;
    while (numDevices-- != 0u)
    {
        refDevices.emplace_back(static_cast<Device *>(*devices++));
        binaryVec.emplace_back(*lengths++);
        std::memcpy(binaryVec.back().data(), *binaries++, binaryVec.back().size());
    }
    return createProgram(
        new Program(*this, std::move(refDevices), std::move(binaryVec), binaryStatus, errcodeRet),
        errcodeRet);
}

cl_program Context::createProgramWithBuiltInKernels(cl_uint numDevices,
                                                    const cl_device_id *devices,
                                                    const char *kernelNames,
                                                    cl_int *errcodeRet)
{
    DeviceRefList refDevices;
    while (numDevices-- != 0u)
    {
        refDevices.emplace_back(static_cast<Device *>(*devices++));
    }
    return createProgram(new Program(*this, std::move(refDevices), kernelNames, errcodeRet),
                         errcodeRet);
}

bool Context::IsValid(const _cl_context *context)
{
    const Platform::PtrList &platforms = Platform::GetPlatforms();
    return std::find_if(platforms.cbegin(), platforms.cend(), [=](const PlatformPtr &platform) {
               return platform->hasContext(context);
           }) != platforms.cend();
}

Context::Context(Platform &platform,
                 PropArray &&properties,
                 DeviceRefList &&devices,
                 ContextErrorCB notify,
                 void *userData,
                 bool userSync,
                 cl_int *errcodeRet)
    : _cl_context(platform.getDispatch()),
      mPlatform(platform),
      mImpl(
          platform.mImpl->createContext(*this, devices, ErrorCallback, this, userSync, errcodeRet)),
      mProperties(std::move(properties)),
      mDevices(std::move(devices)),
      mNotify(notify),
      mUserData(userData)
{}

Context::Context(Platform &platform,
                 PropArray &&properties,
                 cl_device_type deviceType,
                 ContextErrorCB notify,
                 void *userData,
                 bool userSync,
                 cl_int *errcodeRet)
    : _cl_context(platform.getDispatch()),
      mPlatform(platform),
      mImpl(platform.mImpl->createContextFromType(*this,
                                                  deviceType,
                                                  ErrorCallback,
                                                  this,
                                                  userSync,
                                                  errcodeRet)),
      mProperties(std::move(properties)),
      mDevices(mImpl ? mImpl->getDevices() : DeviceRefList{}),
      mNotify(notify),
      mUserData(userData)
{}

cl_command_queue Context::createCommandQueue(CommandQueue *commandQueue, cl_int *errcodeRet)
{
    mCommandQueues.emplace_back(commandQueue);
    if (!mCommandQueues.back()->mImpl)
    {
        mCommandQueues.back()->release();
        return nullptr;
    }
    if (errcodeRet != nullptr)
    {
        *errcodeRet = CL_SUCCESS;
    }
    return mCommandQueues.back().get();
}

cl_mem Context::createMemory(Memory *memory, cl_int *errcodeRet)
{
    mMemories.emplace_back(memory);
    if (!mMemories.back()->mImpl || mMemories.back()->mSize == 0u)
    {
        mMemories.back()->release();
        return nullptr;
    }
    if (errcodeRet != nullptr)
    {
        *errcodeRet = CL_SUCCESS;
    }
    return mMemories.back().get();
}

cl_sampler Context::createSampler(Sampler *sampler, cl_int *errcodeRet)
{
    mSamplers.emplace_back(sampler);
    if (!mSamplers.back()->mImpl)
    {
        mSamplers.back()->release();
        return nullptr;
    }
    if (errcodeRet != nullptr)
    {
        *errcodeRet = CL_SUCCESS;
    }
    return mSamplers.back().get();
}

cl_program Context::createProgram(Program *program, cl_int *errcodeRet)
{
    mPrograms.emplace_back(program);
    if (!mPrograms.back()->mImpl)
    {
        mPrograms.back()->release();
        return nullptr;
    }
    if (errcodeRet != nullptr)
    {
        *errcodeRet = CL_SUCCESS;
    }
    return mPrograms.back().get();
}

void Context::destroyCommandQueue(CommandQueue *commandQueue)
{
    auto commandQueueIt = mCommandQueues.cbegin();
    while (commandQueueIt != mCommandQueues.cend() && commandQueueIt->get() != commandQueue)
    {
        ++commandQueueIt;
    }
    if (commandQueueIt != mCommandQueues.cend())
    {
        mCommandQueues.erase(commandQueueIt);
    }
    else
    {
        ERR() << "CommandQueue not found";
    }
}

void Context::destroyMemory(Memory *memory)
{
    auto memoryIt = mMemories.cbegin();
    while (memoryIt != mMemories.cend() && memoryIt->get() != memory)
    {
        ++memoryIt;
    }
    if (memoryIt != mMemories.cend())
    {
        mMemories.erase(memoryIt);
    }
    else
    {
        ERR() << "Memory not found";
    }
}

void Context::destroySampler(Sampler *sampler)
{
    auto samplerIt = mSamplers.cbegin();
    while (samplerIt != mSamplers.cend() && samplerIt->get() != sampler)
    {
        ++samplerIt;
    }
    if (samplerIt != mSamplers.cend())
    {
        mSamplers.erase(samplerIt);
    }
    else
    {
        ERR() << "Sampler not found";
    }
}

void Context::destroyProgram(Program *program)
{
    auto programIt = mPrograms.cbegin();
    while (programIt != mPrograms.cend() && programIt->get() != program)
    {
        ++programIt;
    }
    if (programIt != mPrograms.cend())
    {
        mPrograms.erase(programIt);
    }
    else
    {
        ERR() << "Program not found";
    }
}

void Context::ErrorCallback(const char *errinfo, const void *privateInfo, size_t cb, void *userData)
{
    Context *const context = static_cast<Context *>(userData);
    if (!Context::IsValid(context))
    {
        WARN() << "Context error for invalid context";
        return;
    }
    if (context->mNotify != nullptr)
    {
        context->mNotify(errinfo, privateInfo, cb, context->mUserData);
    }
}

}  // namespace cl
