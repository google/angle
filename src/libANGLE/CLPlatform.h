//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatform.h: Defines the cl::Platform class, which provides information about platform-specific
// OpenCL features.

#ifndef LIBANGLE_CLPLATFORM_H_
#define LIBANGLE_CLPLATFORM_H_

#include "libANGLE/CLObject.h"
#include "libANGLE/renderer/CLPlatformImpl.h"

#include "anglebase/no_destructor.h"

#include <algorithm>
#include <string>

namespace cl
{

class Platform final : public _cl_platform_id, public Object
{
  public:
    using Ptr  = std::unique_ptr<Platform>;
    using List = std::vector<Ptr>;

    ~Platform();

    const char *getProfile() const;
    const char *getVersionString() const;
    cl_version getVersion() const;
    const char *getName() const;
    const char *getExtensions() const;
    const rx::CLPlatformImpl::ExtensionList &getExtensionsWithVersion() const;
    cl_ulong getHostTimerResolution() const;

    static void CreatePlatform(const cl_icd_dispatch &dispatch, rx::CLPlatformImpl::Ptr &&impl);
    static const List &GetPlatforms();
    static bool IsValid(const Platform *platform);
    static constexpr const char *GetVendor();
    static constexpr const char *GetIcdSuffix();

  private:
    Platform(const cl_icd_dispatch &dispatch, rx::CLPlatformImpl::Ptr &&impl);

    static List &GetList();

    const rx::CLPlatformImpl::Ptr mImpl;

    static constexpr char kVendor[]    = "ANGLE";
    static constexpr char kIcdSuffix[] = "ANGLE";
};

inline const char *Platform::getProfile() const
{
    return mImpl->getInfo().mProfile.c_str();
}

inline const char *Platform::getVersionString() const
{
    return mImpl->getInfo().mVersionStr.c_str();
}

inline cl_version Platform::getVersion() const
{
    return mImpl->getInfo().mVersion;
}

inline const char *Platform::getName() const
{
    return mImpl->getInfo().mName.c_str();
}

inline const char *Platform::getExtensions() const
{
    return mImpl->getInfo().mExtensions.c_str();
}

inline const rx::CLPlatformImpl::ExtensionList &Platform::getExtensionsWithVersion() const
{
    return mImpl->getInfo().mExtensionList;
}

inline cl_ulong Platform::getHostTimerResolution() const
{
    return mImpl->getInfo().mHostTimerRes;
}

inline Platform::List &Platform::GetList()
{
    static angle::base::NoDestructor<List> sList;
    return *sList;
}

inline const Platform::List &Platform::GetPlatforms()
{
    return GetList();
}

inline bool Platform::IsValid(const Platform *platform)
{
    const List &platforms = GetPlatforms();
    return std::find_if(platforms.cbegin(), platforms.cend(),
                        [=](const Ptr &ptr) { return ptr.get() == platform; }) != platforms.cend();
}

constexpr const char *Platform::GetVendor()
{
    return kVendor;
}

constexpr const char *Platform::GetIcdSuffix()
{
    return kIcdSuffix;
}

}  // namespace cl

#endif  // LIBANGLE_CLPLATFORM_H_
