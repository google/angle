//
// Copyright (c) 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ShaderCompileTreeTest.cpp:
//   Test that shader validation results in the correct compile status.
//

#include "tests/test_utils/ShaderCompileTreeTest.h"

#include "compiler/translator/TranslatorESSL.h"

using namespace sh;

void ShaderCompileTreeTest::SetUp()
{
    mAllocator.push();
    SetGlobalPoolAllocator(&mAllocator);

    ShBuiltInResources resources;
    sh::InitBuiltInResources(&resources);

    initResources(&resources);

    mTranslator = new TranslatorESSL(getShaderType(), getShaderSpec());
    ASSERT_TRUE(mTranslator->Init(resources));
}

void ShaderCompileTreeTest::TearDown()
{
    delete mTranslator;

    SetGlobalPoolAllocator(nullptr);
    mAllocator.pop();
}

bool ShaderCompileTreeTest::compile(const std::string &shaderString)
{
    const char *shaderStrings[] = {shaderString.c_str()};
    mASTRoot = mTranslator->compileTreeForTesting(shaderStrings, 1, mExtraCompileOptions);
    TInfoSink &infoSink = mTranslator->getInfoSink();
    mInfoLog            = infoSink.info.c_str();
    return mASTRoot != nullptr;
}

void ShaderCompileTreeTest::compileAssumeSuccess(const std::string &shaderString)
{
    if (!compile(shaderString))
    {
        FAIL() << "Shader compilation into ESSL failed, log:\n" << mInfoLog;
    }
}

bool ShaderCompileTreeTest::hasWarning() const
{
    return mInfoLog.find("WARNING: ") != std::string::npos;
}

const std::vector<sh::Uniform> ShaderCompileTreeTest::getUniforms()
{
    ASSERT(mExtraCompileOptions & SH_VARIABLES);
    return mTranslator->getUniforms();
}