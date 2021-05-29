//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLContextCL.h: Defines the class interface for CLContextCL, implementing CLContextImpl.

#ifndef LIBANGLE_RENDERER_CL_CLCONTEXTCL_H_
#define LIBANGLE_RENDERER_CL_CLCONTEXTCL_H_

#include "libANGLE/renderer/CLContextImpl.h"

namespace rx
{

class CLContextCL : public CLContextImpl
{
  public:
    CLContextCL(const cl::Context &context, cl_context native);
    ~CLContextCL() override;

    cl::DevicePtrs getDevices(cl_int &errorCode) const override;

    CLCommandQueueImpl::Ptr createCommandQueue(const cl::CommandQueue &commandQueue,
                                               cl_int &errorCode) override;

    CLMemoryImpl::Ptr createBuffer(const cl::Buffer &buffer,
                                   size_t size,
                                   void *hostPtr,
                                   cl_int &errorCode) override;

    CLMemoryImpl::Ptr createImage(const cl::Image &image,
                                  const cl_image_format &format,
                                  const cl::ImageDescriptor &desc,
                                  void *hostPtr,
                                  cl_int &errorCode) override;

    CLSamplerImpl::Ptr createSampler(const cl::Sampler &sampler, cl_int &errorCode) override;

    CLProgramImpl::Ptr createProgramWithSource(const cl::Program &program,
                                               const std::string &source,
                                               cl_int &errorCode) override;

    CLProgramImpl::Ptr createProgramWithIL(const cl::Program &program,
                                           const void *il,
                                           size_t length,
                                           cl_int &errorCode) override;

    CLProgramImpl::Ptr createProgramWithBinary(const cl::Program &program,
                                               const cl::Binaries &binaries,
                                               cl_int *binaryStatus,
                                               cl_int &errorCode) override;

    CLProgramImpl::Ptr createProgramWithBuiltInKernels(const cl::Program &program,
                                                       const char *kernel_names,
                                                       cl_int &errorCode) override;

    CLEventImpl::Ptr createUserEvent(const cl::Event &event, cl_int &errorCode) override;

    cl_int waitForEvents(const cl::EventPtrs &events) override;

  private:
    const cl_context mNative;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CL_CLCONTEXTCL_H_
