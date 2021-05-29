//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLProgram.h: Defines the cl::Program class, which consists of a set of OpenCL kernels.

#ifndef LIBANGLE_CLPROGRAM_H_
#define LIBANGLE_CLPROGRAM_H_

#include "libANGLE/CLKernel.h"
#include "libANGLE/renderer/CLProgramImpl.h"

namespace cl
{

class Program final : public _cl_program, public Object
{
  public:
    ~Program() override;

    Context &getContext();
    const Context &getContext() const;
    const DevicePtrs &getDevices() const;

    template <typename T = rx::CLProgramImpl>
    T &getImpl() const;

    cl_int getInfo(ProgramInfo name, size_t valueSize, void *value, size_t *valueSizeRet) const;

    cl_kernel createKernel(const char *kernel_name, cl_int &errorCode);

    cl_int createKernels(cl_uint numKernels, cl_kernel *kernels, cl_uint *numKernelsRet);

  private:
    Program(Context &context, std::string &&source, cl_int &errorCode);
    Program(Context &context, const void *il, size_t length, cl_int &errorCode);

    Program(Context &context,
            DevicePtrs &&devices,
            Binaries &&binaries,
            cl_int *binaryStatus,
            cl_int &errorCode);

    Program(Context &context, DevicePtrs &&devices, const char *kernelNames, cl_int &errorCode);

    const ContextPtr mContext;
    const DevicePtrs mDevices;
    const std::string mIL;
    const rx::CLProgramImpl::Ptr mImpl;
    const std::string mSource;

    Binaries mBinaries;
    size_t mNumKernels;
    std::string mKernelNames;

    friend class Object;
};

inline Context &Program::getContext()
{
    return *mContext;
}

inline const Context &Program::getContext() const
{
    return *mContext;
}

inline const DevicePtrs &Program::getDevices() const
{
    return mDevices;
}

template <typename T>
inline T &Program::getImpl() const
{
    return static_cast<T &>(*mImpl);
}

}  // namespace cl

#endif  // LIBANGLE_CLPROGRAM_H_
