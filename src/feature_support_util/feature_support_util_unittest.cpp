//
// Copyright (c) 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// feature_support_util_unittest.cpp: Unit test for the feature-support utility.

#include <gtest/gtest.h>

#include "../gpu_info_util/SystemInfo.h"
#include "feature_support_util.h"

using namespace angle;

// Test the ANGLEGetFeatureSupportUtilAPIVersion function
TEST(FeatureSupportUtilTest, APIVersion)
{
    unsigned int versionToUse;
    unsigned int zero = 0;

    versionToUse = kFeatureVersion_LowestSupported;
    EXPECT_TRUE(ANGLEGetFeatureSupportUtilAPIVersion(&versionToUse));
    EXPECT_EQ(kFeatureVersion_LowestSupported, versionToUse);

    versionToUse = kFeatureVersion_HighestSupported;
    EXPECT_TRUE(ANGLEGetFeatureSupportUtilAPIVersion(&versionToUse));
    EXPECT_EQ(kFeatureVersion_HighestSupported, versionToUse);

    versionToUse = zero;
    EXPECT_FALSE(ANGLEGetFeatureSupportUtilAPIVersion(&versionToUse));
    EXPECT_EQ(zero, versionToUse);

    versionToUse = kFeatureVersion_HighestSupported + 1;
    EXPECT_TRUE(ANGLEGetFeatureSupportUtilAPIVersion(&versionToUse));
    EXPECT_EQ(kFeatureVersion_HighestSupported, versionToUse);
}

// Test the ANGLEAddDeviceInfoToSystemInfo function
TEST(FeatureSupportUtilTest, SystemInfo)
{
    // TODO(ianelliott): Replace this with a gtest "fixture", per review feedback.
    SystemInfo systemInfo;
    systemInfo.machineManufacturer = "BAD";
    systemInfo.machineModelName    = "BAD";
    systemInfo.gpus.resize(1);
    systemInfo.gpus[0].vendorId              = 123;
    systemInfo.gpus[0].deviceId              = 234;
    systemInfo.gpus[0].driverVendor          = "DriverVendorA";
    systemInfo.gpus[0].detailedDriverVersion = {1, 2, 3, 4};

    char mfr[]   = "Google";
    char model[] = "Pixel1";

    ANGLEAddDeviceInfoToSystemInfo(mfr, model, &systemInfo);
    EXPECT_EQ("Google", systemInfo.machineManufacturer);
    EXPECT_EQ("Pixel1", systemInfo.machineModelName);
}

// Test the ANGLEAndroidParseRulesString function
TEST(FeatureSupportUtilTest, ParseRules)
{
    // TODO(ianelliott): Replace this with a gtest "fixture", per review feedback.
    SystemInfo systemInfo;
    systemInfo.machineManufacturer = "Google";
    systemInfo.machineModelName    = "Pixel1";
    systemInfo.gpus.resize(1);
    systemInfo.gpus[0].vendorId              = 123;
    systemInfo.gpus[0].deviceId              = 234;
    systemInfo.gpus[0].driverVendor          = "DriverVendorA";
    systemInfo.gpus[0].detailedDriverVersion = {1, 2, 3, 4};

    constexpr char rulesFileContents[] =
        "{\"Rules\":[{\"Rule\":\"Default Rule (i.e. use native driver)\", \"AppChoice\":true, "
        "\"NonChoice\":false}]}\n";
    RulesHandle rulesHandle = nullptr;
    int rulesVersion        = 0;
    EXPECT_TRUE(ANGLEAndroidParseRulesString(rulesFileContents, &rulesHandle, &rulesVersion));
    EXPECT_NE(nullptr, rulesHandle);
    ANGLEFreeRulesHandle(rulesHandle);
}
