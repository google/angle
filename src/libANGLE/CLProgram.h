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
    using PtrList = std::list<ProgramPtr>;

    ~Program() override;

    Context &getContext();
    const Context &getContext() const;
    const DeviceRefs &getDevices() const;
    const Kernel::PtrList &getKernels() const;

    bool hasKernel(const _cl_kernel *kernel) const;

    void retain() noexcept;
    bool release();

    cl_int getInfo(ProgramInfo name, size_t valueSize, void *value, size_t *valueSizeRet) const;

    cl_kernel createKernel(const char *kernel_name, cl_int &errorCode);
    cl_int createKernel(const Kernel::CreateImplFunc &createImplFunc);
    cl_int createKernels(cl_uint numKernels, cl_kernel *kernels, cl_uint *numKernelsRet);

    static bool IsValid(const _cl_program *program);

  private:
    Program(Context &context, std::string &&source, cl_int &errorCode);
    Program(Context &context, const void *il, size_t length, cl_int &errorCode);

    Program(Context &context,
            DeviceRefs &&devices,
            Binaries &&binaries,
            cl_int *binaryStatus,
            cl_int &errorCode);

    Program(Context &context, DeviceRefs &&devices, const char *kernelNames, cl_int &errorCode);

    cl_kernel createKernel(Kernel *kernel);

    void destroyKernel(Kernel *kernel);

    const ContextRefPtr mContext;
    const DeviceRefs mDevices;
    const std::string mIL;
    const rx::CLProgramImpl::Ptr mImpl;
    const std::string mSource;

    Binaries mBinaries;
    size_t mNumKernels;
    std::string mKernelNames;
    Kernel::PtrList mKernels;

    friend class Context;
    friend class Kernel;
};

inline Context &Program::getContext()
{
    return *mContext;
}

inline const Context &Program::getContext() const
{
    return *mContext;
}

inline const DeviceRefs &Program::getDevices() const
{
    return mDevices;
}

inline const Kernel::PtrList &Program::getKernels() const
{
    return mKernels;
}

inline bool Program::hasKernel(const _cl_kernel *kernel) const
{
    return std::find_if(mKernels.cbegin(), mKernels.cend(), [=](const KernelPtr &ptr) {
               return ptr.get() == kernel;
           }) != mKernels.cend();
}

inline void Program::retain() noexcept
{
    addRef();
}

}  // namespace cl

#endif  // LIBANGLE_CLPROGRAM_H_
