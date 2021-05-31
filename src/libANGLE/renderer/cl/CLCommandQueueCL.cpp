//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLCommandQueueCL.cpp: Implements the class methods for CLCommandQueueCL.

#include "libANGLE/renderer/cl/CLCommandQueueCL.h"

#include "libANGLE/renderer/cl/CLEventCL.h"
#include "libANGLE/renderer/cl/CLMemoryCL.h"

#include "libANGLE/CLBuffer.h"

namespace rx
{

CLCommandQueueCL::CLCommandQueueCL(const cl::CommandQueue &commandQueue, cl_command_queue native)
    : CLCommandQueueImpl(commandQueue), mNative(native)
{}

CLCommandQueueCL::~CLCommandQueueCL()
{
    if (mNative->getDispatch().clReleaseCommandQueue(mNative) != CL_SUCCESS)
    {
        ERR() << "Error while releasing CL command-queue";
    }
}

cl_int CLCommandQueueCL::setProperty(cl::CommandQueueProperties properties, cl_bool enable)
{
    return mNative->getDispatch().clSetCommandQueueProperty(mNative, properties.get(), enable,
                                                            nullptr);
}

cl_int CLCommandQueueCL::enqueueReadBuffer(const cl::Buffer &buffer,
                                           bool blocking,
                                           size_t offset,
                                           size_t size,
                                           void *ptr,
                                           const cl::EventPtrs &waitEvents,
                                           CLEventImpl::CreateFunc *eventCreateFunc)
{
    const cl_mem nativeBuffer                = buffer.getImpl<CLMemoryCL>().getNative();
    const cl_bool block                      = blocking ? CL_TRUE : CL_FALSE;
    const std::vector<cl_event> nativeEvents = CLEventCL::Cast(waitEvents);
    const cl_uint numEvents                  = static_cast<cl_uint>(nativeEvents.size());
    const cl_event *const nativeEventsPtr    = nativeEvents.empty() ? nullptr : nativeEvents.data();
    cl_event nativeEvent                     = nullptr;
    cl_event *const nativeEventPtr           = eventCreateFunc != nullptr ? &nativeEvent : nullptr;

    const cl_int errorCode =
        mNative->getDispatch().clEnqueueReadBuffer(mNative, nativeBuffer, block, offset, size, ptr,
                                                   numEvents, nativeEventsPtr, nativeEventPtr);

    if (errorCode == CL_SUCCESS && eventCreateFunc != nullptr)
    {
        *eventCreateFunc = [=](const cl::Event &event) {
            return CLEventImpl::Ptr(new CLEventCL(event, nativeEvent));
        };
    }
    return errorCode;
}

cl_int CLCommandQueueCL::enqueueWriteBuffer(const cl::Buffer &buffer,
                                            bool blocking,
                                            size_t offset,
                                            size_t size,
                                            const void *ptr,
                                            const cl::EventPtrs &waitEvents,
                                            CLEventImpl::CreateFunc *eventCreateFunc)
{
    const cl_mem nativeBuffer                = buffer.getImpl<CLMemoryCL>().getNative();
    const cl_bool block                      = blocking ? CL_TRUE : CL_FALSE;
    const std::vector<cl_event> nativeEvents = CLEventCL::Cast(waitEvents);
    const cl_uint numEvents                  = static_cast<cl_uint>(nativeEvents.size());
    const cl_event *const nativeEventsPtr    = nativeEvents.empty() ? nullptr : nativeEvents.data();
    cl_event nativeEvent                     = nullptr;
    cl_event *const nativeEventPtr           = eventCreateFunc != nullptr ? &nativeEvent : nullptr;

    const cl_int errorCode =
        mNative->getDispatch().clEnqueueWriteBuffer(mNative, nativeBuffer, block, offset, size, ptr,
                                                    numEvents, nativeEventsPtr, nativeEventPtr);

    if (errorCode == CL_SUCCESS && eventCreateFunc != nullptr)
    {
        *eventCreateFunc = [=](const cl::Event &event) {
            return CLEventImpl::Ptr(new CLEventCL(event, nativeEvent));
        };
    }
    return errorCode;
}

cl_int CLCommandQueueCL::enqueueReadBufferRect(const cl::Buffer &buffer,
                                               bool blocking,
                                               const size_t bufferOrigin[3],
                                               const size_t hostOrigin[3],
                                               const size_t region[3],
                                               size_t bufferRowPitch,
                                               size_t bufferSlicePitch,
                                               size_t hostRowPitch,
                                               size_t hostSlicePitch,
                                               void *ptr,
                                               const cl::EventPtrs &waitEvents,
                                               CLEventImpl::CreateFunc *eventCreateFunc)
{
    const cl_mem nativeBuffer                = buffer.getImpl<CLMemoryCL>().getNative();
    const cl_bool block                      = blocking ? CL_TRUE : CL_FALSE;
    const std::vector<cl_event> nativeEvents = CLEventCL::Cast(waitEvents);
    const cl_uint numEvents                  = static_cast<cl_uint>(nativeEvents.size());
    const cl_event *const nativeEventsPtr    = nativeEvents.empty() ? nullptr : nativeEvents.data();
    cl_event nativeEvent                     = nullptr;
    cl_event *const nativeEventPtr           = eventCreateFunc != nullptr ? &nativeEvent : nullptr;

    const cl_int errorCode = mNative->getDispatch().clEnqueueReadBufferRect(
        mNative, nativeBuffer, block, bufferOrigin, hostOrigin, region, bufferRowPitch,
        bufferSlicePitch, hostRowPitch, hostSlicePitch, ptr, numEvents, nativeEventsPtr,
        nativeEventPtr);

    if (errorCode == CL_SUCCESS && eventCreateFunc != nullptr)
    {
        *eventCreateFunc = [=](const cl::Event &event) {
            return CLEventImpl::Ptr(new CLEventCL(event, nativeEvent));
        };
    }
    return errorCode;
}

cl_int CLCommandQueueCL::enqueueWriteBufferRect(const cl::Buffer &buffer,
                                                bool blocking,
                                                const size_t bufferOrigin[3],
                                                const size_t hostOrigin[3],
                                                const size_t region[3],
                                                size_t bufferRowPitch,
                                                size_t bufferSlicePitch,
                                                size_t hostRowPitch,
                                                size_t hostSlicePitch,
                                                const void *ptr,
                                                const cl::EventPtrs &waitEvents,
                                                CLEventImpl::CreateFunc *eventCreateFunc)
{
    const cl_mem nativeBuffer                = buffer.getImpl<CLMemoryCL>().getNative();
    const cl_bool block                      = blocking ? CL_TRUE : CL_FALSE;
    const std::vector<cl_event> nativeEvents = CLEventCL::Cast(waitEvents);
    const cl_uint numEvents                  = static_cast<cl_uint>(nativeEvents.size());
    const cl_event *const nativeEventsPtr    = nativeEvents.empty() ? nullptr : nativeEvents.data();
    cl_event nativeEvent                     = nullptr;
    cl_event *const nativeEventPtr           = eventCreateFunc != nullptr ? &nativeEvent : nullptr;

    const cl_int errorCode = mNative->getDispatch().clEnqueueWriteBufferRect(
        mNative, nativeBuffer, block, bufferOrigin, hostOrigin, region, bufferRowPitch,
        bufferSlicePitch, hostRowPitch, hostSlicePitch, ptr, numEvents, nativeEventsPtr,
        nativeEventPtr);

    if (errorCode == CL_SUCCESS && eventCreateFunc != nullptr)
    {
        *eventCreateFunc = [=](const cl::Event &event) {
            return CLEventImpl::Ptr(new CLEventCL(event, nativeEvent));
        };
    }
    return errorCode;
}

cl_int CLCommandQueueCL::enqueueCopyBuffer(const cl::Buffer &srcBuffer,
                                           const cl::Buffer &dstBuffer,
                                           size_t srcOffset,
                                           size_t dstOffset,
                                           size_t size,
                                           const cl::EventPtrs &waitEvents,
                                           CLEventImpl::CreateFunc *eventCreateFunc)
{
    const cl_mem nativeSrc                   = srcBuffer.getImpl<CLMemoryCL>().getNative();
    const cl_mem nativeDst                   = dstBuffer.getImpl<CLMemoryCL>().getNative();
    const std::vector<cl_event> nativeEvents = CLEventCL::Cast(waitEvents);
    const cl_uint numEvents                  = static_cast<cl_uint>(nativeEvents.size());
    const cl_event *const nativeEventsPtr    = nativeEvents.empty() ? nullptr : nativeEvents.data();
    cl_event nativeEvent                     = nullptr;
    cl_event *const nativeEventPtr           = eventCreateFunc != nullptr ? &nativeEvent : nullptr;

    const cl_int errorCode = mNative->getDispatch().clEnqueueCopyBuffer(
        mNative, nativeSrc, nativeDst, srcOffset, dstOffset, size, numEvents, nativeEventsPtr,
        nativeEventPtr);

    if (errorCode == CL_SUCCESS && eventCreateFunc != nullptr)
    {
        *eventCreateFunc = [=](const cl::Event &event) {
            return CLEventImpl::Ptr(new CLEventCL(event, nativeEvent));
        };
    }
    return errorCode;
}

cl_int CLCommandQueueCL::enqueueCopyBufferRect(const cl::Buffer &srcBuffer,
                                               const cl::Buffer &dstBuffer,
                                               const size_t srcOrigin[3],
                                               const size_t dstOrigin[3],
                                               const size_t region[3],
                                               size_t srcRowPitch,
                                               size_t srcSlicePitch,
                                               size_t dstRowPitch,
                                               size_t dstSlicePitch,
                                               const cl::EventPtrs &waitEvents,
                                               CLEventImpl::CreateFunc *eventCreateFunc)
{
    const cl_mem nativeSrc                   = srcBuffer.getImpl<CLMemoryCL>().getNative();
    const cl_mem nativeDst                   = dstBuffer.getImpl<CLMemoryCL>().getNative();
    const std::vector<cl_event> nativeEvents = CLEventCL::Cast(waitEvents);
    const cl_uint numEvents                  = static_cast<cl_uint>(nativeEvents.size());
    const cl_event *const nativeEventsPtr    = nativeEvents.empty() ? nullptr : nativeEvents.data();
    cl_event nativeEvent                     = nullptr;
    cl_event *const nativeEventPtr           = eventCreateFunc != nullptr ? &nativeEvent : nullptr;

    const cl_int errorCode = mNative->getDispatch().clEnqueueCopyBufferRect(
        mNative, nativeSrc, nativeDst, srcOrigin, dstOrigin, region, srcRowPitch, srcSlicePitch,
        dstRowPitch, dstSlicePitch, numEvents, nativeEventsPtr, nativeEventPtr);

    if (errorCode == CL_SUCCESS && eventCreateFunc != nullptr)
    {
        *eventCreateFunc = [=](const cl::Event &event) {
            return CLEventImpl::Ptr(new CLEventCL(event, nativeEvent));
        };
    }
    return errorCode;
}

cl_int CLCommandQueueCL::enqueueFillBuffer(const cl::Buffer &buffer,
                                           const void *pattern,
                                           size_t patternSize,
                                           size_t offset,
                                           size_t size,
                                           const cl::EventPtrs &waitEvents,
                                           CLEventImpl::CreateFunc *eventCreateFunc)
{
    const cl_mem nativeBuffer                = buffer.getImpl<CLMemoryCL>().getNative();
    const std::vector<cl_event> nativeEvents = CLEventCL::Cast(waitEvents);
    const cl_uint numEvents                  = static_cast<cl_uint>(nativeEvents.size());
    const cl_event *const nativeEventsPtr    = nativeEvents.empty() ? nullptr : nativeEvents.data();
    cl_event nativeEvent                     = nullptr;
    cl_event *const nativeEventPtr           = eventCreateFunc != nullptr ? &nativeEvent : nullptr;

    const cl_int errorCode = mNative->getDispatch().clEnqueueFillBuffer(
        mNative, nativeBuffer, pattern, patternSize, offset, size, numEvents, nativeEventsPtr,
        nativeEventPtr);

    if (errorCode == CL_SUCCESS && eventCreateFunc != nullptr)
    {
        *eventCreateFunc = [=](const cl::Event &event) {
            return CLEventImpl::Ptr(new CLEventCL(event, nativeEvent));
        };
    }
    return errorCode;
}

void *CLCommandQueueCL::enqueueMapBuffer(const cl::Buffer &buffer,
                                         bool blocking,
                                         cl::MapFlags mapFlags,
                                         size_t offset,
                                         size_t size,
                                         const cl::EventPtrs &waitEvents,
                                         CLEventImpl::CreateFunc *eventCreateFunc,
                                         cl_int &errorCode)
{
    const cl_mem nativeBuffer                = buffer.getImpl<CLMemoryCL>().getNative();
    const cl_bool block                      = blocking ? CL_TRUE : CL_FALSE;
    const std::vector<cl_event> nativeEvents = CLEventCL::Cast(waitEvents);
    const cl_uint numEvents                  = static_cast<cl_uint>(nativeEvents.size());
    const cl_event *const nativeEventsPtr    = nativeEvents.empty() ? nullptr : nativeEvents.data();
    cl_event nativeEvent                     = nullptr;
    cl_event *const nativeEventPtr           = eventCreateFunc != nullptr ? &nativeEvent : nullptr;

    void *const region = mNative->getDispatch().clEnqueueMapBuffer(
        mNative, nativeBuffer, block, mapFlags.get(), offset, size, numEvents, nativeEventsPtr,
        nativeEventPtr, &errorCode);

    if (errorCode == CL_SUCCESS && eventCreateFunc != nullptr)
    {
        *eventCreateFunc = [=](const cl::Event &event) {
            return CLEventImpl::Ptr(new CLEventCL(event, nativeEvent));
        };
    }
    return region;
}

}  // namespace rx
