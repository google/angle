// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANGLE_GPU_CONFIG_GPU_TEST_CONFIG_H_
#define ANGLE_GPU_CONFIG_GPU_TEST_CONFIG_H_

#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>

#include "common/debug.h"
#include "common/string_utils.h"

#define DCHECK_EQ(A, B) ASSERT((A) == (B))
#define DCHECK_NE(A, B) ASSERT((A) != (B))
#define DCHECK(X) ASSERT(X)
#define DLOG(X) std::cerr
#define DVLOG(X) std::cerr
#define LOG(X) std::cerr

#define GPU_EXPORT

// Shim Chromium's types by importing symbols in the correct namespaces
namespace base
{
using angle::HexStringToUInt;
using angle::KEEP_WHITESPACE;
using angle::kWhitespaceASCII;
using angle::ReadFileToString;
using angle::SPLIT_WANT_ALL;
using angle::SPLIT_WANT_NONEMPTY;
using angle::SplitString;
using angle::SplitStringAlongWhitespace;
using angle::TRIM_WHITESPACE;

using TimeDelta = int;
}  // namespace base

namespace gfx
{
class Size
{
  public:
    int width() const { return 0; }
    int height() const { return 0; }
    std::string ToString() const { return "0x0"; }
};
}  // namespace gfx

struct DxDiagNode
{};

// TODO(jmadill): other platforms
// clang-format off
#if defined(_WIN32) || defined(_WIN64)
#    define OS_WIN
#elif defined(ANDROID)
#    define OS_ANDROID
#elif defined(__linux__)
#    define OS_LINUX
#elif defined(__APPLE__)
#    define OS_MACOSX
#elif defined(__Fuchsia__)
#    define OS_FUCHSIA
#else
#    error "Unsupported platform"
#endif
// clang-format on

namespace gpu
{

struct GPUInfo;

class GPU_EXPORT GPUTestConfig
{
  public:
    enum OS
    {
        kOsUnknown         = 0,
        kOsWinXP           = 1 << 0,
        kOsWinVista        = 1 << 1,
        kOsWin7            = 1 << 2,
        kOsWin8            = 1 << 3,
        kOsMacLeopard      = 1 << 4,
        kOsMacSnowLeopard  = 1 << 5,
        kOsMacLion         = 1 << 6,
        kOsMacMountainLion = 1 << 7,
        kOsMacMavericks    = 1 << 8,
        kOsMacYosemite     = 1 << 9,
        kOsMacElCapitan    = 1 << 10,
        kOsMacSierra       = 1 << 11,
        kOsMacHighSierra   = 1 << 12,
        kOsMacMojave       = 1 << 13,
        kOsMac             = kOsMacLeopard | kOsMacSnowLeopard | kOsMacLion | kOsMacMountainLion |
                 kOsMacMavericks | kOsMacYosemite | kOsMacElCapitan | kOsMacSierra |
                 kOsMacHighSierra | kOsMacMojave,
        kOsLinux    = 1 << 14,
        kOsChromeOS = 1 << 15,
        kOsAndroid  = 1 << 16,
        kOsWin10    = 1 << 17,
        kOsWin      = kOsWinXP | kOsWinVista | kOsWin7 | kOsWin8 | kOsWin10,
        kOsFuchsia  = 1 << 18,
    };

    enum BuildType
    {
        kBuildTypeUnknown = 0,
        kBuildTypeRelease = 1 << 0,
        kBuildTypeDebug   = 1 << 1,
    };

    enum API
    {
        kAPIUnknown   = 0,
        kAPID3D9      = 1 << 0,
        kAPID3D11     = 1 << 1,
        kAPIGLDesktop = 1 << 2,
        kAPIGLES      = 1 << 3,
        kAPIVulkan    = 1 << 4,
    };

    GPUTestConfig();
    GPUTestConfig(const GPUTestConfig &other);
    virtual ~GPUTestConfig();

    void set_os(int32_t os);
    void set_gpu_device_id(uint32_t id);
    void set_build_type(int32_t build_type);
    void set_api(int32_t api);

    virtual void AddGPUVendor(uint32_t gpu_vendor);

    int32_t os() const { return os_; }
    const std::vector<uint32_t> &gpu_vendor() const { return gpu_vendor_; }
    uint32_t gpu_device_id() const { return gpu_device_id_; }
    int32_t build_type() const { return build_type_; }
    int32_t api() const { return api_; }

    // Check if the config is valid. For example, if gpu_device_id_ is set, but
    // gpu_vendor_ is unknown, then it's invalid.
    virtual bool IsValid() const;

    // Check if two configs overlap, i.e., if there exists a config that matches
    // both configs.
    bool OverlapsWith(const GPUTestConfig &config) const;

    // Disable validation of GPU vendor and device ids.
    void DisableGPUInfoValidation();

  protected:
    void ClearGPUVendor();

    // Indicates that the OS has the notion of a numeric GPU vendor and device id
    // and this data should be validated.
    bool validate_gpu_info_;

  private:
    // operating system.
    int32_t os_;

    // GPU vendor.
    std::vector<uint32_t> gpu_vendor_;

    // GPU device id (unique to each vendor).
    uint32_t gpu_device_id_;

    // Release or Debug.
    int32_t build_type_;

    // Back-end rendering APIs.
    int32_t api_;
};

class GPU_EXPORT GPUTestBotConfig : public GPUTestConfig
{
  public:
    GPUTestBotConfig() = default;
    ~GPUTestBotConfig() override;

    // This should only be called when no gpu_vendor is added.
    void AddGPUVendor(uint32_t gpu_vendor) override;

    // Return false if gpu_info does not have valid vendor_id and device_id.
    bool SetGPUInfo(const GPUInfo &gpu_info);

    // Check if the bot config is valid, i.e., if it is one valid test-bot
    // environment. For example, if a field is unknown, or if OS is not one
    // fully defined OS, then it's valid.
    bool IsValid() const override;

    // Check if a bot config matches a test config, i.e., the test config is a
    // superset of the bot config.
    bool Matches(const GPUTestConfig &config) const;
    bool Matches(const std::string &config_data) const;

    // Setup the config with the current gpu testing environment.
    // If gpu_info is nullptr, collect GPUInfo first.
    bool LoadCurrentConfig(const GPUInfo *gpu_info);

    // Check if this bot's config matches |config_data| or any of the |configs|.
    static bool CurrentConfigMatches(const std::string &config_data);
    static bool CurrentConfigMatches(const std::vector<std::string> &configs);

    // Check if the bot has blacklisted all GPU features.
    static bool GpuBlacklistedOnBot();
};

}  // namespace gpu

#endif  // ANGLE_GPU_CONFIG_GPU_TEST_CONFIG_H_
