//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLCommandQueueCL.h: Defines the class interface for CLCommandQueueCL,
// implementing CLCommandQueueImpl.

#ifndef LIBANGLE_RENDERER_CL_CLCOMMANDQUEUECL_H_
#define LIBANGLE_RENDERER_CL_CLCOMMANDQUEUECL_H_

#include "libANGLE/renderer/CLCommandQueueImpl.h"

namespace rx
{

class CLCommandQueueCL : public CLCommandQueueImpl
{
  public:
    CLCommandQueueCL(const cl::CommandQueue &commandQueue, cl_command_queue native);
    ~CLCommandQueueCL() override;

    cl_int setProperty(cl::CommandQueueProperties properties, cl_bool enable) override;

    cl_int enqueueReadBuffer(const cl::Buffer &buffer,
                             bool blocking,
                             size_t offset,
                             size_t size,
                             void *ptr,
                             const cl::EventPtrs &waitEvents,
                             CLEventImpl::CreateFunc *eventCreateFunc) override;

    cl_int enqueueWriteBuffer(const cl::Buffer &buffer,
                              bool blocking,
                              size_t offset,
                              size_t size,
                              const void *ptr,
                              const cl::EventPtrs &waitEvents,
                              CLEventImpl::CreateFunc *eventCreateFunc) override;

    cl_int enqueueReadBufferRect(const cl::Buffer &buffer,
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
                                 CLEventImpl::CreateFunc *eventCreateFunc) override;

    cl_int enqueueWriteBufferRect(const cl::Buffer &buffer,
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
                                  CLEventImpl::CreateFunc *eventCreateFunc) override;

    cl_int enqueueCopyBuffer(const cl::Buffer &srcBuffer,
                             const cl::Buffer &dstBuffer,
                             size_t srcOffset,
                             size_t dstOffset,
                             size_t size,
                             const cl::EventPtrs &waitEvents,
                             CLEventImpl::CreateFunc *eventCreateFunc) override;

    cl_int enqueueCopyBufferRect(const cl::Buffer &srcBuffer,
                                 const cl::Buffer &dstBuffer,
                                 const size_t srcOrigin[3],
                                 const size_t dstOrigin[3],
                                 const size_t region[3],
                                 size_t srcRowPitch,
                                 size_t srcSlicePitch,
                                 size_t dstRowPitch,
                                 size_t dstSlicePitch,
                                 const cl::EventPtrs &waitEvents,
                                 CLEventImpl::CreateFunc *eventCreateFunc) override;

    cl_int enqueueFillBuffer(const cl::Buffer &buffer,
                             const void *pattern,
                             size_t patternSize,
                             size_t offset,
                             size_t size,
                             const cl::EventPtrs &waitEvents,
                             CLEventImpl::CreateFunc *eventCreateFunc) override;

    void *enqueueMapBuffer(const cl::Buffer &buffer,
                           bool blocking,
                           cl::MapFlags mapFlags,
                           size_t offset,
                           size_t size,
                           const cl::EventPtrs &waitEvents,
                           CLEventImpl::CreateFunc *eventCreateFunc,
                           cl_int &errorCode) override;

  private:
    const cl_command_queue mNative;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CL_CLCOMMANDQUEUECL_H_
