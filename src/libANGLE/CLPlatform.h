//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatform.h: Defines the cl::Platform class, which provides information about platform-specific
// OpenCL features.

#ifndef LIBANGLE_CLPLATFORM_H_
#define LIBANGLE_CLPLATFORM_H_

#include "libANGLE/CLContext.h"
#include "libANGLE/CLDevice.h"
#include "libANGLE/renderer/CLPlatformImpl.h"

#include "anglebase/no_destructor.h"

#include <functional>

namespace cl
{

class Platform final : public _cl_platform_id, public Object
{
  public:
    using PtrList        = std::list<PlatformPtr>;
    using CreateImplFunc = std::function<rx::CLPlatformImpl::Ptr(const cl::Platform &)>;

    ~Platform();

    const rx::CLPlatformImpl::Info &getInfo() const;
    bool hasDevice(const _cl_device_id *device) const;
    const DevicePtrList &getDevices() const;
    bool hasContext(const _cl_context *context) const;

    cl_int getInfo(PlatformInfo name, size_t valueSize, void *value, size_t *valueSizeRet);

    cl_int getDeviceIDs(cl_device_type deviceType,
                        cl_uint numEntries,
                        cl_device_id *devices,
                        cl_uint *numDevices) const;

    static void CreatePlatform(const cl_icd_dispatch &dispatch,
                               const CreateImplFunc &createImplFunc);

    static cl_int GetPlatformIDs(cl_uint num_entries,
                                 cl_platform_id *platforms,
                                 cl_uint *num_platforms);

    static cl_context CreateContext(const cl_context_properties *properties,
                                    cl_uint numDevices,
                                    const cl_device_id *devices,
                                    ContextErrorCB notify,
                                    void *userData,
                                    cl_int *errcodeRet);

    static cl_context CreateContextFromType(const cl_context_properties *properties,
                                            cl_device_type deviceType,
                                            ContextErrorCB notify,
                                            void *userData,
                                            cl_int *errcodeRet);

    static const PtrList &GetPlatforms();
    static Platform *GetDefault();
    static Platform *CastOrDefault(cl_platform_id platform);
    static bool IsValid(const _cl_platform_id *platform);
    static bool IsValidOrDefault(const _cl_platform_id *platform);

    static constexpr const char *GetVendor();

  private:
    Platform(const cl_icd_dispatch &dispatch, const CreateImplFunc &createImplFunc);

    void destroyContext(Context *context);

    static PtrList &GetList();

    const rx::CLPlatformImpl::Ptr mImpl;
    const rx::CLPlatformImpl::Info mInfo;
    const DevicePtrList mDevices;

    Context::PtrList mContexts;

    static constexpr char kVendor[]    = "ANGLE";
    static constexpr char kIcdSuffix[] = "ANGLE";

    friend class Context;
};

inline const rx::CLPlatformImpl::Info &Platform::getInfo() const
{
    return mInfo;
}

inline bool Platform::hasDevice(const _cl_device_id *device) const
{
    return std::find_if(mDevices.cbegin(), mDevices.cend(), [=](const DevicePtr &ptr) {
               return ptr.get() == device || ptr->hasSubDevice(device);
           }) != mDevices.cend();
}

inline const DevicePtrList &Platform::getDevices() const
{
    return mDevices;
}

inline bool Platform::hasContext(const _cl_context *context) const
{
    return std::find_if(mContexts.cbegin(), mContexts.cend(), [=](const ContextPtr &ptr) {
               return ptr.get() == context;
           }) != mContexts.cend();
}

inline Platform::PtrList &Platform::GetList()
{
    static angle::base::NoDestructor<PtrList> sList;
    return *sList;
}

inline const Platform::PtrList &Platform::GetPlatforms()
{
    return GetList();
}

inline Platform *Platform::GetDefault()
{
    return GetList().empty() ? nullptr : GetList().front().get();
}

inline Platform *Platform::CastOrDefault(cl_platform_id platform)
{
    return platform != nullptr ? static_cast<Platform *>(platform) : GetDefault();
}

inline bool Platform::IsValid(const _cl_platform_id *platform)
{
    const PtrList &platforms = GetPlatforms();
    return std::find_if(platforms.cbegin(), platforms.cend(), [=](const PlatformPtr &ptr) {
               return ptr.get() == platform;
           }) != platforms.cend();
}

// Our CL implementation defines that a nullptr value chooses the platform that we provide as
// default, so this function returns true for a nullptr value if a default platform exists.
inline bool Platform::IsValidOrDefault(const _cl_platform_id *platform)
{
    return platform != nullptr ? IsValid(platform) : GetDefault() != nullptr;
}

constexpr const char *Platform::GetVendor()
{
    return kVendor;
}

}  // namespace cl

#endif  // LIBANGLE_CLPLATFORM_H_
