//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// angle_deqp_gtest:
//   dEQP and GoogleTest integration logic. Calls through to the random
//   order executor.

#include <stdint.h>
#include <zlib.h>

#include <gtest/gtest.h>

#include "angle_deqp_libtester.h"
#include "common/angleutils.h"
#include "common/debug.h"
#include "gpu_test_expectations_parser.h"
#include "system_utils.h"

namespace
{

class dEQPCaseList
{
  public:
    dEQPCaseList(const std::string &caseListPath, const std::string &testExpectationsPath);

    struct CaseInfo
    {
        CaseInfo(const std::string &dEQPName,
                 const std::string &gTestName,
                 int expectation)
            : mDEQPName(dEQPName),
              mGTestName(gTestName),
              mExpectation(expectation)
        {
        }

        std::string mDEQPName;
        std::string mGTestName;
        int mExpectation;
    };

    const CaseInfo &getCaseInfo(size_t caseIndex) const
    {
        ASSERT(caseIndex < mCaseInfoList.size());
        return mCaseInfoList[caseIndex];
    }

    size_t numCases() const
    {
        return mCaseInfoList.size();
    }

    static dEQPCaseList *GetInstance();
    static void FreeInstance();

  private:
    std::vector<CaseInfo> mCaseInfoList;
    gpu::GPUTestExpectationsParser mTestExpectationsParser;
    gpu::GPUTestBotConfig mTestConfig;

    static dEQPCaseList *mInstance;
};

// static
dEQPCaseList *dEQPCaseList::mInstance = nullptr;

// static
dEQPCaseList *dEQPCaseList::GetInstance()
{
    if (mInstance == nullptr)
    {
        std::string exeDir = angle::GetExecutableDirectory();
        std::string caseListPath = exeDir + "/deqp_support/dEQP-GLES2-cases.txt.gz";
        std::string testExpectationsPath = exeDir + "/deqp_support/deqp_test_expectations.txt";
        mInstance = new dEQPCaseList(caseListPath, testExpectationsPath);
    }
    return mInstance;
}

// Not currently called, assumed to be freed on program exit.
// static
void dEQPCaseList::FreeInstance()
{
    SafeDelete(mInstance);
}

dEQPCaseList::dEQPCaseList(const std::string &caseListPath, const std::string &testExpectationsPath)
{
    if (!mTestExpectationsParser.LoadTestExpectationsFromFile(testExpectationsPath))
    {
        std::cerr << "Failed to load test expectations." << std::endl;
        for (const auto &message : mTestExpectationsParser.GetErrorMessages())
        {
            std::cerr << " " << message << std::endl;
        }
        return;
    }

    if (!mTestConfig.LoadCurrentConfig(nullptr))
    {
        std::cerr << "Failed to load test configuration." << std::endl;
        return;
    }

    std::stringstream caseListStream;

    std::vector<char> buf(1024 * 1024 * 16);
    gzFile *fi = static_cast<gzFile *>(gzopen(caseListPath.c_str(), "rb"));

    if (fi == nullptr)
    {
        return;
    }

    gzrewind(fi);
    while (!gzeof(fi))
    {
        int len = gzread(fi, &buf[0], static_cast<unsigned int>(buf.size()) - 1);
        buf[len] = '\0';
        caseListStream << &buf[0];
    }
    gzclose(fi);

    while (!caseListStream.eof())
    {
        std::string inString;
        std::getline(caseListStream, inString);

        if (inString.substr(0, 4) == "TEST")
        {
            std::string dEQPName = inString.substr(6);
            std::string gTestName = dEQPName.substr(11);
            std::replace(gTestName.begin(), gTestName.end(), '.', '_');

            // Occurs in some luminance tests
            gTestName.erase(std::remove(gTestName.begin(), gTestName.end(), '-'), gTestName.end());

            int expectation = mTestExpectationsParser.GetTestExpectation(dEQPName, mTestConfig);
            if (expectation != gpu::GPUTestExpectationsParser::kGpuTestSkip)
            {
                mCaseInfoList.push_back(CaseInfo(dEQPName, gTestName, expectation));
            }
        }
    }
}

class dEQP_GLES2 : public testing::TestWithParam<size_t>
{
  protected:
    dEQP_GLES2() {}

    void runTest()
    {
        const auto &caseInfo = dEQPCaseList::GetInstance()->getCaseInfo(GetParam());
        std::cout << caseInfo.mDEQPName << std::endl;

        bool result = deqp_libtester_run(caseInfo.mDEQPName.c_str());
        if (caseInfo.mExpectation == gpu::GPUTestExpectationsParser::kGpuTestPass)
        {
            EXPECT_TRUE(result);
        }
        else if (result)
        {
            std::cout << "Test expected to fail but passed!" << std::endl;
        }
    }
};

// TODO(jmadill): add different platform configs, or ability to choose platform
TEST_P(dEQP_GLES2, Default)
{
    runTest();
}

testing::internal::ParamGenerator<size_t> GetTestingRange()
{
    return testing::Range<size_t>(0, dEQPCaseList::GetInstance()->numCases());
}

INSTANTIATE_TEST_CASE_P(, dEQP_GLES2, GetTestingRange());

} // anonymous namespace
