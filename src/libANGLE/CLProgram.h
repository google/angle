//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLProgram.h: Defines the cl::Program class, which consists of a set of OpenCL kernels.

#ifndef LIBANGLE_CLPROGRAM_H_
#define LIBANGLE_CLPROGRAM_H_

#include "libANGLE/CLObject.h"
#include "libANGLE/renderer/CLProgramImpl.h"

namespace cl
{

class Program final : public _cl_program, public Object
{
  public:
    using PtrList = std::list<ProgramPtr>;

    ~Program() override;

    const Context &getContext() const;
    const DeviceRefList &getDevices() const;

    void retain() noexcept;
    bool release();

    cl_int getInfo(ProgramInfo name, size_t valueSize, void *value, size_t *valueSizeRet) const;

    static bool IsValid(const _cl_program *program);

  private:
    Program(Context &context, std::string &&source, cl_int *errcodeRet);
    Program(Context &context, const void *il, size_t length, cl_int *errcodeRet);

    Program(Context &context,
            DeviceRefList &&devices,
            Binaries &&binaries,
            cl_int *binaryStatus,
            cl_int *errcodeRet);

    Program(Context &context, DeviceRefList &&devices, const char *kernelNames, cl_int *errcodeRet);

    const ContextRefPtr mContext;
    const DeviceRefList mDevices;
    const std::string mIL;
    const rx::CLProgramImpl::Ptr mImpl;
    const std::string mSource;

    Binaries mBinaries;

    friend class Context;
};

inline const Context &Program::getContext() const
{
    return *mContext;
}

inline const DeviceRefList &Program::getDevices() const
{
    return mDevices;
}

inline void Program::retain() noexcept
{
    addRef();
}

}  // namespace cl

#endif  // LIBANGLE_CLPROGRAM_H_
