//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLContextCL.cpp: Implements the class methods for CLContextCL.

#include "libANGLE/renderer/cl/CLContextCL.h"

#include "libANGLE/renderer/cl/CLCommandQueueCL.h"
#include "libANGLE/renderer/cl/CLDeviceCL.h"
#include "libANGLE/renderer/cl/CLMemoryCL.h"
#include "libANGLE/renderer/cl/CLProgramCL.h"
#include "libANGLE/renderer/cl/CLSamplerCL.h"

#include "libANGLE/CLBuffer.h"
#include "libANGLE/CLCommandQueue.h"
#include "libANGLE/CLContext.h"
#include "libANGLE/CLDevice.h"
#include "libANGLE/CLImage.h"
#include "libANGLE/CLMemory.h"
#include "libANGLE/CLPlatform.h"
#include "libANGLE/CLProgram.h"
#include "libANGLE/CLSampler.h"
#include "libANGLE/Debug.h"

namespace rx
{

CLContextCL::CLContextCL(const cl::Context &context, cl_context native)
    : CLContextImpl(context), mNative(native)
{}

CLContextCL::~CLContextCL()
{
    if (mNative->getDispatch().clReleaseContext(mNative) != CL_SUCCESS)
    {
        ERR() << "Error while releasing CL context";
    }
}

cl::DeviceRefList CLContextCL::getDevices() const
{
    size_t valueSize = 0u;
    cl_int result    = mNative->getDispatch().clGetContextInfo(mNative, CL_CONTEXT_DEVICES, 0u,
                                                            nullptr, &valueSize);
    if (result == CL_SUCCESS && (valueSize % sizeof(cl_device_id)) == 0u)
    {
        std::vector<cl_device_id> nativeDevices(valueSize / sizeof(cl_device_id), nullptr);
        result = mNative->getDispatch().clGetContextInfo(mNative, CL_CONTEXT_DEVICES, valueSize,
                                                         nativeDevices.data(), nullptr);
        if (result == CL_SUCCESS)
        {
            const cl::DevicePtrList &platformDevices = mContext.getPlatform().getDevices();
            cl::DeviceRefList devices;
            for (cl_device_id nativeDevice : nativeDevices)
            {
                auto it = platformDevices.cbegin();
                while (it != platformDevices.cend() &&
                       (*it)->getImpl<CLDeviceCL &>().getNative() != nativeDevice)
                {
                    ++it;
                }
                if (it != platformDevices.cend())
                {
                    devices.emplace_back(it->get());
                }
                else
                {
                    ERR() << "Device not found in platform list";
                    return cl::DeviceRefList{};
                }
            }
            return devices;
        }
    }

    ERR() << "Error fetching devices from CL context, code: " << result;
    return cl::DeviceRefList{};
}

CLCommandQueueImpl::Ptr CLContextCL::createCommandQueue(const cl::CommandQueue &commandQueue,
                                                        cl_int *errcodeRet)
{
    const cl::Device &device        = commandQueue.getDevice();
    const cl_device_id nativeDevice = device.getImpl<CLDeviceCL &>().getNative();
    cl_command_queue nativeQueue    = nullptr;
    if (!device.isVersionOrNewer(2u, 0u))
    {
        nativeQueue = mNative->getDispatch().clCreateCommandQueue(
            mNative, nativeDevice, commandQueue.getProperties(), errcodeRet);
    }
    else
    {
        const cl_queue_properties propArray[] = {CL_QUEUE_PROPERTIES, commandQueue.getProperties(),
                                                 commandQueue.hasSize() ? CL_QUEUE_SIZE : 0u,
                                                 commandQueue.getSize(), 0u};
        nativeQueue = mNative->getDispatch().clCreateCommandQueueWithProperties(
            mNative, nativeDevice, propArray, errcodeRet);
    }
    return CLCommandQueueImpl::Ptr(
        nativeQueue != nullptr ? new CLCommandQueueCL(commandQueue, nativeQueue) : nullptr);
}

CLMemoryImpl::Ptr CLContextCL::createBuffer(const cl::Buffer &buffer,
                                            size_t size,
                                            void *hostPtr,
                                            cl_int *errcodeRet)
{
    cl_mem nativeBuffer = nullptr;
    if (buffer.getProperties().empty())
    {
        nativeBuffer = mNative->getDispatch().clCreateBuffer(mNative, buffer.getFlags(), size,
                                                             hostPtr, errcodeRet);
    }
    else
    {
        nativeBuffer = mNative->getDispatch().clCreateBufferWithProperties(
            mNative, buffer.getProperties().data(), buffer.getFlags(), size, hostPtr, errcodeRet);
    }
    return CLMemoryImpl::Ptr(nativeBuffer != nullptr ? new CLMemoryCL(buffer, nativeBuffer)
                                                     : nullptr);
}

CLMemoryImpl::Ptr CLContextCL::createImage(const cl::Image &image,
                                           const cl_image_format &format,
                                           const cl::ImageDescriptor &desc,
                                           void *hostPtr,
                                           cl_int *errcodeRet)
{
    cl_mem nativeImage = nullptr;

    if (mContext.getPlatform().isVersionOrNewer(1u, 2u))
    {
        const cl_image_desc nativeDesc = {
            desc.type,       desc.width,
            desc.height,     desc.depth,
            desc.arraySize,  desc.rowPitch,
            desc.slicePitch, desc.numMipLevels,
            desc.numSamples, {static_cast<cl_mem>(image.getParent().get())}};

        if (image.getProperties().empty())
        {
            nativeImage = mNative->getDispatch().clCreateImage(mNative, image.getFlags(), &format,
                                                               &nativeDesc, hostPtr, errcodeRet);
        }
        else
        {
            nativeImage = mNative->getDispatch().clCreateImageWithProperties(
                mNative, image.getProperties().data(), image.getFlags(), &format, &nativeDesc,
                hostPtr, errcodeRet);
        }
    }
    else
    {
        switch (desc.type)
        {
            case CL_MEM_OBJECT_IMAGE2D:
                nativeImage = mNative->getDispatch().clCreateImage2D(
                    mNative, image.getFlags(), &format, desc.width, desc.height, desc.rowPitch,
                    hostPtr, errcodeRet);
                break;
            case CL_MEM_OBJECT_IMAGE3D:
                nativeImage = mNative->getDispatch().clCreateImage3D(
                    mNative, image.getFlags(), &format, desc.width, desc.height, desc.depth,
                    desc.rowPitch, desc.slicePitch, hostPtr, errcodeRet);
                break;
            default:
                ERR() << "Failed to create unsupported image type";
                break;
        }
    }

    return CLMemoryImpl::Ptr(nativeImage != nullptr ? new CLMemoryCL(image, nativeImage) : nullptr);
}

CLSamplerImpl::Ptr CLContextCL::createSampler(const cl::Sampler &sampler, cl_int *errcodeRet)
{
    cl_sampler nativeSampler = nullptr;
    if (!mContext.getPlatform().isVersionOrNewer(2u, 0u))
    {
        nativeSampler = mNative->getDispatch().clCreateSampler(
            mNative, sampler.getNormalizedCoords(), cl::ToCLenum(sampler.getAddressingMode()),
            cl::ToCLenum(sampler.getFilterMode()), errcodeRet);
    }
    else if (!sampler.getProperties().empty())
    {
        nativeSampler = mNative->getDispatch().clCreateSamplerWithProperties(
            mNative, sampler.getProperties().data(), errcodeRet);
    }
    else
    {
        const cl_sampler_properties propArray[] = {CL_SAMPLER_NORMALIZED_COORDS,
                                                   sampler.getNormalizedCoords(),
                                                   CL_SAMPLER_ADDRESSING_MODE,
                                                   cl::ToCLenum(sampler.getAddressingMode()),
                                                   CL_SAMPLER_FILTER_MODE,
                                                   cl::ToCLenum(sampler.getFilterMode()),
                                                   0u};
        nativeSampler =
            mNative->getDispatch().clCreateSamplerWithProperties(mNative, propArray, errcodeRet);
    }
    return CLSamplerImpl::Ptr(nativeSampler != nullptr ? new CLSamplerCL(sampler, nativeSampler)
                                                       : nullptr);
}

CLProgramImpl::Ptr CLContextCL::createProgramWithSource(const cl::Program &program,
                                                        const std::string &source,
                                                        cl_int *errcodeRet)
{
    const char *sourceStr          = source.c_str();
    const size_t length            = source.length();
    const cl_program nativeProgram = mNative->getDispatch().clCreateProgramWithSource(
        mNative, 1u, &sourceStr, &length, errcodeRet);
    return CLProgramImpl::Ptr(nativeProgram != nullptr ? new CLProgramCL(program, nativeProgram)
                                                       : nullptr);
}

CLProgramImpl::Ptr CLContextCL::createProgramWithIL(const cl::Program &program,
                                                    const void *il,
                                                    size_t length,
                                                    cl_int *errcodeRet)
{
    const cl_program nativeProgram =
        mNative->getDispatch().clCreateProgramWithIL(mNative, il, length, errcodeRet);
    return CLProgramImpl::Ptr(nativeProgram != nullptr ? new CLProgramCL(program, nativeProgram)
                                                       : nullptr);
}

CLProgramImpl::Ptr CLContextCL::createProgramWithBinary(const cl::Program &program,
                                                        const cl::Binaries &binaries,
                                                        cl_int *binaryStatus,
                                                        cl_int *errcodeRet)
{
    ASSERT(program.getDevices().size() == binaries.size());
    std::vector<cl_device_id> nativeDevices;
    for (const cl::DeviceRefPtr &device : program.getDevices())
    {
        nativeDevices.emplace_back(device->getImpl<CLDeviceCL &>().getNative());
    }
    std::vector<size_t> lengths;
    std::vector<const unsigned char *> nativeBinaries;
    for (const cl::Binary &binary : binaries)
    {
        lengths.emplace_back(binary.size());
        nativeBinaries.emplace_back(binary.data());
    }
    const cl_program nativeProgram = mNative->getDispatch().clCreateProgramWithBinary(
        mNative, static_cast<cl_uint>(nativeDevices.size()), nativeDevices.data(), lengths.data(),
        nativeBinaries.data(), binaryStatus, errcodeRet);
    return CLProgramImpl::Ptr(nativeProgram != nullptr ? new CLProgramCL(program, nativeProgram)
                                                       : nullptr);
}

CLProgramImpl::Ptr CLContextCL::createProgramWithBuiltInKernels(const cl::Program &program,
                                                                const char *kernel_names,
                                                                cl_int *errcodeRet)
{
    std::vector<cl_device_id> nativeDevices;
    for (const cl::DeviceRefPtr &device : program.getDevices())
    {
        nativeDevices.emplace_back(device->getImpl<CLDeviceCL &>().getNative());
    }
    const cl_program nativeProgram = mNative->getDispatch().clCreateProgramWithBuiltInKernels(
        mNative, static_cast<cl_uint>(nativeDevices.size()), nativeDevices.data(), kernel_names,
        errcodeRet);
    return CLProgramImpl::Ptr(nativeProgram != nullptr ? new CLProgramCL(program, nativeProgram)
                                                       : nullptr);
}

}  // namespace rx
