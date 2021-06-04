//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLCommandQueue.cpp: Implements the cl::CommandQueue class.

#include "libANGLE/CLCommandQueue.h"

#include "libANGLE/CLBuffer.h"
#include "libANGLE/CLContext.h"
#include "libANGLE/CLDevice.h"
#include "libANGLE/CLEvent.h"
#include "libANGLE/CLImage.h"

#include <cstring>

namespace cl
{

cl_int CommandQueue::getInfo(CommandQueueInfo name,
                             size_t valueSize,
                             void *value,
                             size_t *valueSizeRet) const
{
    cl_uint valUInt       = 0u;
    void *valPointer      = nullptr;
    const void *copyValue = nullptr;
    size_t copySize       = 0u;

    switch (name)
    {
        case CommandQueueInfo::Context:
            valPointer = mContext->getNative();
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case CommandQueueInfo::Device:
            valPointer = mDevice->getNative();
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case CommandQueueInfo::ReferenceCount:
            valUInt   = getRefCount();
            copyValue = &valUInt;
            copySize  = sizeof(valUInt);
            break;
        case CommandQueueInfo::Properties:
            copyValue = &mProperties;
            copySize  = sizeof(mProperties);
            break;
        case CommandQueueInfo::PropertiesArray:
            copyValue = mPropArray.data();
            copySize  = mPropArray.size() * sizeof(decltype(mPropArray)::value_type);
            break;
        case CommandQueueInfo::Size:
            copyValue = &mSize;
            copySize  = sizeof(mSize);
            break;
        case CommandQueueInfo::DeviceDefault:
            valPointer = CommandQueue::CastNative(mDevice->mDefaultCommandQueue);
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        default:
            return CL_INVALID_VALUE;
    }

    if (value != nullptr)
    {
        // CL_INVALID_VALUE if size in bytes specified by param_value_size is < size of return type
        // as specified in the Command Queue Parameter table, and param_value is not a NULL value.
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

cl_int CommandQueue::setProperty(CommandQueueProperties properties,
                                 cl_bool enable,
                                 cl_command_queue_properties *oldProperties)
{
    if (oldProperties != nullptr)
    {
        *oldProperties = mProperties.get();
    }
    const cl_int result = mImpl->setProperty(properties, enable);
    if (result == CL_SUCCESS)
    {
        if (enable == CL_FALSE)
        {
            mProperties.clear(properties);
        }
        else
        {
            mProperties.set(properties);
        }
    }
    return result;
}

cl_int CommandQueue::enqueueReadBuffer(cl_mem buffer,
                                       cl_bool blockingRead,
                                       size_t offset,
                                       size_t size,
                                       void *ptr,
                                       cl_uint numEventsInWaitList,
                                       const cl_event *eventWaitList,
                                       cl_event *event)
{
    const Buffer &buf          = buffer->cast<Buffer>();
    const bool blocking        = blockingRead != CL_FALSE;
    const EventPtrs waitEvents = Event::Cast(numEventsInWaitList, eventWaitList);
    rx::CLEventImpl::CreateFunc eventCreateFunc;
    rx::CLEventImpl::CreateFunc *const eventCreateFuncPtr =
        event != nullptr ? &eventCreateFunc : nullptr;

    cl_int errorCode =
        mImpl->enqueueReadBuffer(buf, blocking, offset, size, ptr, waitEvents, eventCreateFuncPtr);

    if (errorCode == CL_SUCCESS && event != nullptr)
    {
        ASSERT(eventCreateFunc);
        *event = Object::Create<Event>(errorCode, *this, CL_COMMAND_READ_BUFFER, eventCreateFunc);
    }
    return errorCode;
}

cl_int CommandQueue::enqueueWriteBuffer(cl_mem buffer,
                                        cl_bool blockingWrite,
                                        size_t offset,
                                        size_t size,
                                        const void *ptr,
                                        cl_uint numEventsInWaitList,
                                        const cl_event *eventWaitList,
                                        cl_event *event)
{
    const Buffer &buf          = buffer->cast<Buffer>();
    const bool blocking        = blockingWrite != CL_FALSE;
    const EventPtrs waitEvents = Event::Cast(numEventsInWaitList, eventWaitList);
    rx::CLEventImpl::CreateFunc eventCreateFunc;
    rx::CLEventImpl::CreateFunc *const eventCreateFuncPtr =
        event != nullptr ? &eventCreateFunc : nullptr;

    cl_int errorCode =
        mImpl->enqueueWriteBuffer(buf, blocking, offset, size, ptr, waitEvents, eventCreateFuncPtr);

    if (errorCode == CL_SUCCESS && event != nullptr)
    {
        ASSERT(eventCreateFunc);
        *event = Object::Create<Event>(errorCode, *this, CL_COMMAND_WRITE_BUFFER, eventCreateFunc);
    }
    return errorCode;
}

cl_int CommandQueue::enqueueReadBufferRect(cl_mem buffer,
                                           cl_bool blockingRead,
                                           const size_t *bufferOrigin,
                                           const size_t *hostOrigin,
                                           const size_t *region,
                                           size_t bufferRowPitch,
                                           size_t bufferSlicePitch,
                                           size_t hostRowPitch,
                                           size_t hostSlicePitch,
                                           void *ptr,
                                           cl_uint numEventsInWaitList,
                                           const cl_event *eventWaitList,
                                           cl_event *event)
{
    const Buffer &buf          = buffer->cast<Buffer>();
    const bool blocking        = blockingRead != CL_FALSE;
    const EventPtrs waitEvents = Event::Cast(numEventsInWaitList, eventWaitList);
    rx::CLEventImpl::CreateFunc eventCreateFunc;
    rx::CLEventImpl::CreateFunc *const eventCreateFuncPtr =
        event != nullptr ? &eventCreateFunc : nullptr;

    cl_int errorCode = mImpl->enqueueReadBufferRect(
        buf, blocking, bufferOrigin, hostOrigin, region, bufferRowPitch, bufferSlicePitch,
        hostRowPitch, hostSlicePitch, ptr, waitEvents, eventCreateFuncPtr);

    if (errorCode == CL_SUCCESS && event != nullptr)
    {
        ASSERT(eventCreateFunc);
        *event =
            Object::Create<Event>(errorCode, *this, CL_COMMAND_READ_BUFFER_RECT, eventCreateFunc);
    }
    return errorCode;
}

cl_int CommandQueue::enqueueWriteBufferRect(cl_mem buffer,
                                            cl_bool blockingWrite,
                                            const size_t *bufferOrigin,
                                            const size_t *hostOrigin,
                                            const size_t *region,
                                            size_t bufferRowPitch,
                                            size_t bufferSlicePitch,
                                            size_t hostRowPitch,
                                            size_t hostSlicePitch,
                                            const void *ptr,
                                            cl_uint numEventsInWaitList,
                                            const cl_event *eventWaitList,
                                            cl_event *event)
{
    const Buffer &buf          = buffer->cast<Buffer>();
    const bool blocking        = blockingWrite != CL_FALSE;
    const EventPtrs waitEvents = Event::Cast(numEventsInWaitList, eventWaitList);
    rx::CLEventImpl::CreateFunc eventCreateFunc;
    rx::CLEventImpl::CreateFunc *const eventCreateFuncPtr =
        event != nullptr ? &eventCreateFunc : nullptr;

    cl_int errorCode = mImpl->enqueueWriteBufferRect(
        buf, blocking, bufferOrigin, hostOrigin, region, bufferRowPitch, bufferSlicePitch,
        hostRowPitch, hostSlicePitch, ptr, waitEvents, eventCreateFuncPtr);

    if (errorCode == CL_SUCCESS && event != nullptr)
    {
        ASSERT(eventCreateFunc);
        *event =
            Object::Create<Event>(errorCode, *this, CL_COMMAND_WRITE_BUFFER_RECT, eventCreateFunc);
    }
    return errorCode;
}

cl_int CommandQueue::enqueueCopyBuffer(cl_mem srcBuffer,
                                       cl_mem dstBuffer,
                                       size_t srcOffset,
                                       size_t dstOffset,
                                       size_t size,
                                       cl_uint numEventsInWaitList,
                                       const cl_event *eventWaitList,
                                       cl_event *event)
{
    const Buffer &src          = srcBuffer->cast<Buffer>();
    const Buffer &dst          = dstBuffer->cast<Buffer>();
    const EventPtrs waitEvents = Event::Cast(numEventsInWaitList, eventWaitList);
    rx::CLEventImpl::CreateFunc eventCreateFunc;
    rx::CLEventImpl::CreateFunc *const eventCreateFuncPtr =
        event != nullptr ? &eventCreateFunc : nullptr;

    cl_int errorCode = mImpl->enqueueCopyBuffer(src, dst, srcOffset, dstOffset, size, waitEvents,
                                                eventCreateFuncPtr);

    if (errorCode == CL_SUCCESS && event != nullptr)
    {
        ASSERT(eventCreateFunc);
        *event = Object::Create<Event>(errorCode, *this, CL_COMMAND_COPY_BUFFER, eventCreateFunc);
    }
    return errorCode;
}

cl_int CommandQueue::enqueueCopyBufferRect(cl_mem srcBuffer,
                                           cl_mem dstBuffer,
                                           const size_t *srcOrigin,
                                           const size_t *dstOrigin,
                                           const size_t *region,
                                           size_t srcRowPitch,
                                           size_t srcSlicePitch,
                                           size_t dstRowPitch,
                                           size_t dstSlicePitch,
                                           cl_uint numEventsInWaitList,
                                           const cl_event *eventWaitList,
                                           cl_event *event)
{
    const Buffer &src          = srcBuffer->cast<Buffer>();
    const Buffer &dst          = dstBuffer->cast<Buffer>();
    const EventPtrs waitEvents = Event::Cast(numEventsInWaitList, eventWaitList);
    rx::CLEventImpl::CreateFunc eventCreateFunc;
    rx::CLEventImpl::CreateFunc *const eventCreateFuncPtr =
        event != nullptr ? &eventCreateFunc : nullptr;

    cl_int errorCode = mImpl->enqueueCopyBufferRect(src, dst, srcOrigin, dstOrigin, region,
                                                    srcRowPitch, srcSlicePitch, dstRowPitch,
                                                    dstSlicePitch, waitEvents, eventCreateFuncPtr);

    if (errorCode == CL_SUCCESS && event != nullptr)
    {
        ASSERT(eventCreateFunc);
        *event =
            Object::Create<Event>(errorCode, *this, CL_COMMAND_COPY_BUFFER_RECT, eventCreateFunc);
    }
    return errorCode;
}

cl_int CommandQueue::enqueueFillBuffer(cl_mem buffer,
                                       const void *pattern,
                                       size_t patternSize,
                                       size_t offset,
                                       size_t size,
                                       cl_uint numEventsInWaitList,
                                       const cl_event *eventWaitList,
                                       cl_event *event)
{
    const Buffer &buf          = buffer->cast<Buffer>();
    const EventPtrs waitEvents = Event::Cast(numEventsInWaitList, eventWaitList);
    rx::CLEventImpl::CreateFunc eventCreateFunc;
    rx::CLEventImpl::CreateFunc *const eventCreateFuncPtr =
        event != nullptr ? &eventCreateFunc : nullptr;

    cl_int errorCode = mImpl->enqueueFillBuffer(buf, pattern, patternSize, offset, size, waitEvents,
                                                eventCreateFuncPtr);

    if (errorCode == CL_SUCCESS && event != nullptr)
    {
        ASSERT(eventCreateFunc);
        *event = Object::Create<Event>(errorCode, *this, CL_COMMAND_FILL_BUFFER, eventCreateFunc);
    }
    return errorCode;
}

void *CommandQueue::enqueueMapBuffer(cl_mem buffer,
                                     cl_bool blockingMap,
                                     MapFlags mapFlags,
                                     size_t offset,
                                     size_t size,
                                     cl_uint numEventsInWaitList,
                                     const cl_event *eventWaitList,
                                     cl_event *event,
                                     cl_int &errorCode)
{
    const Buffer &buf          = buffer->cast<Buffer>();
    const bool blocking        = blockingMap != CL_FALSE;
    const EventPtrs waitEvents = Event::Cast(numEventsInWaitList, eventWaitList);
    rx::CLEventImpl::CreateFunc eventCreateFunc;
    rx::CLEventImpl::CreateFunc *const eventCreateFuncPtr =
        event != nullptr ? &eventCreateFunc : nullptr;

    void *const map = mImpl->enqueueMapBuffer(buf, blocking, mapFlags, offset, size, waitEvents,
                                              eventCreateFuncPtr, errorCode);

    if (errorCode == CL_SUCCESS && event != nullptr)
    {
        ASSERT(eventCreateFunc);
        *event = Object::Create<Event>(errorCode, *this, CL_COMMAND_MAP_BUFFER, eventCreateFunc);
    }
    return map;
}

cl_int CommandQueue::enqueueReadImage(cl_mem image,
                                      cl_bool blockingRead,
                                      const size_t *origin,
                                      const size_t *region,
                                      size_t rowPitch,
                                      size_t slicePitch,
                                      void *ptr,
                                      cl_uint numEventsInWaitList,
                                      const cl_event *eventWaitList,
                                      cl_event *event)
{
    const Image &img           = image->cast<Image>();
    const bool blocking        = blockingRead != CL_FALSE;
    const EventPtrs waitEvents = Event::Cast(numEventsInWaitList, eventWaitList);
    rx::CLEventImpl::CreateFunc eventCreateFunc;
    rx::CLEventImpl::CreateFunc *const eventCreateFuncPtr =
        event != nullptr ? &eventCreateFunc : nullptr;

    cl_int errorCode = mImpl->enqueueReadImage(img, blocking, origin, region, rowPitch, slicePitch,
                                               ptr, waitEvents, eventCreateFuncPtr);

    if (errorCode == CL_SUCCESS && event != nullptr)
    {
        ASSERT(eventCreateFunc);
        *event = Object::Create<Event>(errorCode, *this, CL_COMMAND_READ_IMAGE, eventCreateFunc);
    }
    return errorCode;
}

cl_int CommandQueue::enqueueWriteImage(cl_mem image,
                                       cl_bool blockingWrite,
                                       const size_t *origin,
                                       const size_t *region,
                                       size_t inputRowPitch,
                                       size_t inputSlicePitch,
                                       const void *ptr,
                                       cl_uint numEventsInWaitList,
                                       const cl_event *eventWaitList,
                                       cl_event *event)
{
    const Image &img           = image->cast<Image>();
    const bool blocking        = blockingWrite != CL_FALSE;
    const EventPtrs waitEvents = Event::Cast(numEventsInWaitList, eventWaitList);
    rx::CLEventImpl::CreateFunc eventCreateFunc;
    rx::CLEventImpl::CreateFunc *const eventCreateFuncPtr =
        event != nullptr ? &eventCreateFunc : nullptr;

    cl_int errorCode =
        mImpl->enqueueWriteImage(img, blocking, origin, region, inputRowPitch, inputSlicePitch, ptr,
                                 waitEvents, eventCreateFuncPtr);

    if (errorCode == CL_SUCCESS && event != nullptr)
    {
        ASSERT(eventCreateFunc);
        *event = Object::Create<Event>(errorCode, *this, CL_COMMAND_WRITE_IMAGE, eventCreateFunc);
    }
    return errorCode;
}

cl_int CommandQueue::enqueueCopyImage(cl_mem srcImage,
                                      cl_mem dstImage,
                                      const size_t *srcOrigin,
                                      const size_t *dstOrigin,
                                      const size_t *region,
                                      cl_uint numEventsInWaitList,
                                      const cl_event *eventWaitList,
                                      cl_event *event)
{
    const Image &src           = srcImage->cast<Image>();
    const Image &dst           = dstImage->cast<Image>();
    const EventPtrs waitEvents = Event::Cast(numEventsInWaitList, eventWaitList);
    rx::CLEventImpl::CreateFunc eventCreateFunc;
    rx::CLEventImpl::CreateFunc *const eventCreateFuncPtr =
        event != nullptr ? &eventCreateFunc : nullptr;

    cl_int errorCode = mImpl->enqueueCopyImage(src, dst, srcOrigin, dstOrigin, region, waitEvents,
                                               eventCreateFuncPtr);

    if (errorCode == CL_SUCCESS && event != nullptr)
    {
        ASSERT(eventCreateFunc);
        *event = Object::Create<Event>(errorCode, *this, CL_COMMAND_COPY_IMAGE, eventCreateFunc);
    }
    return errorCode;
}

cl_int CommandQueue::enqueueFillImage(cl_mem image,
                                      const void *fillColor,
                                      const size_t *origin,
                                      const size_t *region,
                                      cl_uint numEventsInWaitList,
                                      const cl_event *eventWaitList,
                                      cl_event *event)
{
    const Image &img           = image->cast<Image>();
    const EventPtrs waitEvents = Event::Cast(numEventsInWaitList, eventWaitList);
    rx::CLEventImpl::CreateFunc eventCreateFunc;
    rx::CLEventImpl::CreateFunc *const eventCreateFuncPtr =
        event != nullptr ? &eventCreateFunc : nullptr;

    cl_int errorCode =
        mImpl->enqueueFillImage(img, fillColor, origin, region, waitEvents, eventCreateFuncPtr);

    if (errorCode == CL_SUCCESS && event != nullptr)
    {
        ASSERT(eventCreateFunc);
        *event = Object::Create<Event>(errorCode, *this, CL_COMMAND_FILL_IMAGE, eventCreateFunc);
    }
    return errorCode;
}

cl_int CommandQueue::enqueueCopyImageToBuffer(cl_mem srcImage,
                                              cl_mem dstBuffer,
                                              const size_t *srcOrigin,
                                              const size_t *region,
                                              size_t dstOffset,
                                              cl_uint numEventsInWaitList,
                                              const cl_event *eventWaitList,
                                              cl_event *event)
{
    const Image &src           = srcImage->cast<Image>();
    const Buffer &dst          = dstBuffer->cast<Buffer>();
    const EventPtrs waitEvents = Event::Cast(numEventsInWaitList, eventWaitList);
    rx::CLEventImpl::CreateFunc eventCreateFunc;
    rx::CLEventImpl::CreateFunc *const eventCreateFuncPtr =
        event != nullptr ? &eventCreateFunc : nullptr;

    cl_int errorCode = mImpl->enqueueCopyImageToBuffer(src, dst, srcOrigin, region, dstOffset,
                                                       waitEvents, eventCreateFuncPtr);

    if (errorCode == CL_SUCCESS && event != nullptr)
    {
        ASSERT(eventCreateFunc);
        *event = Object::Create<Event>(errorCode, *this, CL_COMMAND_COPY_IMAGE_TO_BUFFER,
                                       eventCreateFunc);
    }
    return errorCode;
}

cl_int CommandQueue::enqueueCopyBufferToImage(cl_mem srcBuffer,
                                              cl_mem dstImage,
                                              size_t srcOffset,
                                              const size_t *dstOrigin,
                                              const size_t *region,
                                              cl_uint numEventsInWaitList,
                                              const cl_event *eventWaitList,
                                              cl_event *event)
{
    const Buffer &src          = srcBuffer->cast<Buffer>();
    const Image &dst           = dstImage->cast<Image>();
    const EventPtrs waitEvents = Event::Cast(numEventsInWaitList, eventWaitList);
    rx::CLEventImpl::CreateFunc eventCreateFunc;
    rx::CLEventImpl::CreateFunc *const eventCreateFuncPtr =
        event != nullptr ? &eventCreateFunc : nullptr;

    cl_int errorCode = mImpl->enqueueCopyBufferToImage(src, dst, srcOffset, dstOrigin, region,
                                                       waitEvents, eventCreateFuncPtr);

    if (errorCode == CL_SUCCESS && event != nullptr)
    {
        ASSERT(eventCreateFunc);
        *event = Object::Create<Event>(errorCode, *this, CL_COMMAND_COPY_BUFFER_TO_IMAGE,
                                       eventCreateFunc);
    }
    return errorCode;
}

void *CommandQueue::enqueueMapImage(cl_mem image,
                                    cl_bool blockingMap,
                                    MapFlags mapFlags,
                                    const size_t *origin,
                                    const size_t *region,
                                    size_t *imageRowPitch,
                                    size_t *imageSlicePitch,
                                    cl_uint numEventsInWaitList,
                                    const cl_event *eventWaitList,
                                    cl_event *event,
                                    cl_int &errorCode)
{
    const Image &img           = image->cast<Image>();
    const bool blocking        = blockingMap != CL_FALSE;
    const EventPtrs waitEvents = Event::Cast(numEventsInWaitList, eventWaitList);
    rx::CLEventImpl::CreateFunc eventCreateFunc;
    rx::CLEventImpl::CreateFunc *const eventCreateFuncPtr =
        event != nullptr ? &eventCreateFunc : nullptr;

    void *const map =
        mImpl->enqueueMapImage(img, blocking, mapFlags, origin, region, imageRowPitch,
                               imageSlicePitch, waitEvents, eventCreateFuncPtr, errorCode);

    if (errorCode == CL_SUCCESS && event != nullptr)
    {
        ASSERT(eventCreateFunc);
        *event = Object::Create<Event>(errorCode, *this, CL_COMMAND_MAP_IMAGE, eventCreateFunc);
    }
    return map;
}

CommandQueue::~CommandQueue()
{
    if (mDevice->mDefaultCommandQueue == this)
    {
        mDevice->mDefaultCommandQueue = nullptr;
    }
}

CommandQueue::CommandQueue(Context &context,
                           Device &device,
                           PropArray &&propArray,
                           CommandQueueProperties properties,
                           cl_uint size,
                           cl_int &errorCode)
    : mContext(&context),
      mDevice(&device),
      mPropArray(std::move(propArray)),
      mProperties(properties),
      mSize(size),
      mImpl(context.getImpl().createCommandQueue(*this, errorCode))
{
    if (mProperties.isSet(CL_QUEUE_ON_DEVICE_DEFAULT))
    {
        mDevice->mDefaultCommandQueue = this;
    }
}

CommandQueue::CommandQueue(Context &context,
                           Device &device,
                           CommandQueueProperties properties,
                           cl_int &errorCode)
    : mContext(&context),
      mDevice(&device),
      mProperties(properties),
      mImpl(context.getImpl().createCommandQueue(*this, errorCode))
{}

}  // namespace cl
