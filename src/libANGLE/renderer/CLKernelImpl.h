//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLKernelImpl.h: Defines the abstract rx::CLKernelImpl class.

#ifndef LIBANGLE_RENDERER_CLKERNELIMPL_H_
#define LIBANGLE_RENDERER_CLKERNELIMPL_H_

#include "libANGLE/renderer/CLtypes.h"

namespace rx
{

class CLKernelImpl : angle::NonCopyable
{
  public:
    using Ptr         = std::unique_ptr<CLKernelImpl>;
    using CreateFunc  = std::function<Ptr(const cl::Kernel &)>;
    using CreateFuncs = std::list<CreateFunc>;

    struct WorkGroupInfo
    {
        WorkGroupInfo();
        ~WorkGroupInfo();

        WorkGroupInfo(const WorkGroupInfo &) = delete;
        WorkGroupInfo &operator=(const WorkGroupInfo &) = delete;

        WorkGroupInfo(WorkGroupInfo &&);
        WorkGroupInfo &operator=(WorkGroupInfo &&);

        std::array<size_t, 3u> mGlobalWorkSize       = {};
        size_t mWorkGroupSize                        = 0u;
        std::array<size_t, 3u> mCompileWorkGroupSize = {};
        cl_ulong mLocalMemSize                       = 0u;
        size_t mPrefWorkGroupSizeMultiple            = 0u;
        cl_ulong mPrivateMemSize                     = 0u;
    };

    struct ArgInfo
    {
        ArgInfo();
        ~ArgInfo();

        ArgInfo(const ArgInfo &) = delete;
        ArgInfo &operator=(const ArgInfo &) = delete;

        ArgInfo(ArgInfo &&);
        ArgInfo &operator=(ArgInfo &&);

        bool isAvailable() const { return !mName.empty(); }

        cl_kernel_arg_address_qualifier mAddressQualifier = 0u;
        cl_kernel_arg_access_qualifier mAccessQualifier   = 0u;
        std::string mTypeName;
        cl_kernel_arg_type_qualifier mTypeQualifier = 0u;
        std::string mName;
    };

    struct Info
    {
        Info();
        ~Info();

        Info(const Info &) = delete;
        Info &operator=(const Info &) = delete;

        Info(Info &&);
        Info &operator=(Info &&);

        bool isValid() const { return !mFunctionName.empty(); }

        std::string mFunctionName;
        cl_uint mNumArgs = 0u;
        std::string mAttributes;

        std::vector<WorkGroupInfo> mWorkGroups;
        std::vector<ArgInfo> mArgs;
    };

    CLKernelImpl(const cl::Kernel &kernel);
    virtual ~CLKernelImpl();

    virtual cl_int setArg(cl_uint argIndex, size_t argSize, const void *argValue) = 0;

    virtual Info createInfo(cl_int &errorCode) const = 0;

  protected:
    const cl::Kernel &mKernel;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CLKERNELIMPL_H_
