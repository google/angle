//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TestSuite:
//   Basic implementation of a test harness in ANGLE.

#include "TestSuite.h"

#include <time.h>

#include <gtest/gtest.h>
#include <rapidjson/document.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>

// We directly call into a function to register the parameterized tests. This saves spinning up
// a subprocess with a new gtest filter.
#include "third_party/googletest/src/googletest/src/gtest-internal-inl.h"

namespace js = rapidjson;

namespace tr
{

namespace
{
// FIXME: non-Windows paths
constexpr char kPathSeparator = '\\';
// FIXME: non-Windows environment vars

const char *ParseFlagValue(const char *flag, const char *argument)
{
    if (strstr(argument, flag) == argument)
    {
        return argument + strlen(flag);
    }

    return nullptr;
}

bool ParseIntFlag(const char *flag, const char *argument, int *valueOut)
{
    const char *value = ParseFlagValue(flag, argument);
    if (!value)
    {
        return false;
    }

    char *end            = nullptr;
    const long longValue = strtol(value, &end, 10);

    if (*end != '\0')
    {
        printf("Error parsing integer flag value.\n");
        exit(1);
    }

    if (longValue == LONG_MAX || longValue == LONG_MIN || static_cast<int>(longValue) != longValue)
    {
        printf("OVerflow when parsing integer flag value.\n");
        exit(1);
    }

    *valueOut = static_cast<int>(longValue);
    return true;
}

bool ParseStringFlag(const char *flag, const char *argument, const char **valueOut)
{
    const char *value = ParseFlagValue(flag, argument);
    if (!value)
    {
        return false;
    }

    *valueOut = value;
    return true;
}

void DeleteArg(int *argc, char **argv, int argIndex)
{
    // Shift the remainder of the argv list left by one.  Note that argv has (*argc + 1) elements,
    // the last one always being NULL.  The following loop moves the trailing NULL element as well.
    for (int index = argIndex; index < *argc; ++index)
    {
        argv[index] = argv[index + 1];
    }
    (*argc)--;
}

void AddArg(int *argc, char **argv, const char *arg)
{
    // This unsafe const_cast is necessary to work around gtest limitations.
    argv[*argc]     = const_cast<char *>(arg);
    argv[*argc + 1] = nullptr;
    (*argc)++;
}
}  // namespace

class TestSuite::TestEventListener : public testing::EmptyTestEventListener
{
  public:
    TestEventListener(const char *outputDirectory, const char *testSuiteName)
        : mOutputDirectory(outputDirectory), mTestSuiteName(testSuiteName)
    {}

    void OnTestProgramEnd(const testing::UnitTest &testProgramInfo) override
    {
        time_t ltime;
        time(&ltime);
        struct tm *timeinfo = gmtime(&ltime);
        ltime               = mktime(timeinfo);

        js::Document doc;
        doc.SetObject();

        js::Document::AllocatorType &allocator = doc.GetAllocator();

        int passed = testProgramInfo.successful_test_count();
        int failed = testProgramInfo.failed_test_count();

        js::Value numFailuresByType;
        numFailuresByType.SetObject();
        numFailuresByType.AddMember("PASS", passed, allocator);
        numFailuresByType.AddMember("FAIL", failed, allocator);

        doc.AddMember("interrupted", false, allocator);
        doc.AddMember("path_delimiter", ".", allocator);
        doc.AddMember("version", 3, allocator);
        doc.AddMember("seconds_since_epoch", ltime, allocator);
        doc.AddMember("num_failures_by_type", numFailuresByType, allocator);

        js::Value testSuite;
        testSuite.SetObject();

        for (int i = 0; i < testProgramInfo.total_test_case_count(); ++i)
        {
            const testing::TestCase *testCase = testProgramInfo.GetTestCase(i);
            for (int j = 0; j < testCase->total_test_count(); ++j)
            {
                const testing::TestInfo *testInfo = testCase->GetTestInfo(j);
                const testing::TestResult *result = testInfo->result();

                // Avoid recording info for tests that are not part of the shard.
                if (!testInfo->should_run())
                    continue;

                js::Value jsResult;
                jsResult.SetObject();
                if (result->Passed())
                {
                    jsResult.AddMember("actual", "PASS", allocator);
                }
                else if (result->Failed())
                {
                    jsResult.AddMember("actual", "FAIL", allocator);
                }

                jsResult.AddMember("expected", "PASS", allocator);

                double timeInSeconds = static_cast<double>(result->elapsed_time()) / 1000.0;

                js::Value times;
                times.SetArray();
                times.PushBack(timeInSeconds, allocator);

                jsResult.AddMember("times", times, allocator);

                char testName[500];
                sprintf(testName, "%s.%s", testInfo->test_case_name(), testInfo->name());
                js::Value jsName;
                jsName.SetString(testName, allocator);

                testSuite.AddMember(jsName, jsResult, allocator);
            }
        }

        js::Value tests;
        tests.SetObject();
        tests.AddMember(js::StringRef(mTestSuiteName), testSuite, allocator);

        doc.AddMember("tests", tests, allocator);

        char buf[500];
        sprintf(buf, "%s%c%s", mOutputDirectory, kPathSeparator, "output.json");

        printf("opening %s\n", buf);

        FILE *fp = fopen(buf, "w");

        constexpr size_t kBufferSize = 0xFFFF;
        std::vector<char> writeBuffer(kBufferSize);
        js::FileWriteStream os(fp, writeBuffer.data(), kBufferSize);
        js::PrettyWriter<js::FileWriteStream> writer(os);
        doc.Accept(writer);

        fclose(fp);
    }

  private:
    const char *mOutputDirectory;
    const char *mTestSuiteName;
};

struct TestIdentifier
{
    TestIdentifier() {}
    TestIdentifier(const TestIdentifier &other) = default;

    const char *testCaseName;
    const char *testName;
    const char *file;
    int line = 0;
};

std::vector<TestIdentifier> GetCompiledInTests()
{
    testing::UnitTest *const unitTest = testing::UnitTest::GetInstance();

    std::vector<TestIdentifier> tests;
    for (int i = 0; i < unitTest->total_test_case_count(); ++i)
    {
        const testing::TestCase *testCase = unitTest->GetTestCase(i);
        for (int j = 0; j < testCase->total_test_count(); ++j)
        {
            const testing::TestInfo *testInfo = testCase->GetTestInfo(j);
            TestIdentifier testData;
            testData.testCaseName = testCase->name();
            testData.testName     = testInfo->name();
            testData.file         = testInfo->file();
            testData.line         = testInfo->line();
            tests.push_back(testData);
        }
    }
    return tests;
}

std::string GetTestFilterForShard(const std::vector<TestIdentifier> &tests,
                                  int shardIndex,
                                  int shardCount)
{
    std::stringstream filterStream;
    int testIndex = shardIndex;

    filterStream << "--gtest_filter=";

    while (testIndex < static_cast<int>(tests.size()))
    {
        filterStream << tests[testIndex].testCaseName;
        filterStream << ".";
        filterStream << tests[testIndex].testName;
        testIndex += shardCount;
        if (testIndex < static_cast<int>(tests.size()))
        {
            filterStream << ":";
        }
    }

    return filterStream.str();
}

TestSuite::TestSuite(int *argc, char **argv)
    : mResultsDirectory(nullptr), mShardCount(-1), mShardIndex(-1)
{
    bool hasFilter = false;

    if (*argc <= 0)
    {
        printf("Missing test arguments.\n");
        exit(1);
    }

    parseTestSuiteName(argv[0]);

    for (int argIndex = 1; argIndex < *argc;)
    {
        if (parseTestSuiteFlag(argv[argIndex]))
        {
            DeleteArg(argc, argv, argIndex);
        }
        else
        {
            if (ParseFlagValue("--gtest_filter=", argv[argIndex]))
            {
                hasFilter = true;
            }
            ++argIndex;
        }
    }

    if ((mShardIndex >= 0) != (mShardCount > 1))
    {
        printf("Shard index and shard count must be specified together.\n");
        exit(1);
    }

    if (mShardCount > 0)
    {
        if (hasFilter)
        {
            printf("Cannot use gtest_filter in conjunction with sharding parameters.\n");
            exit(1);
        }

        testing::internal::UnitTestImpl *impl = testing::internal::GetUnitTestImpl();
        impl->RegisterParameterizedTests();

        std::vector<TestIdentifier> tests = GetCompiledInTests();
        mFilterString                     = GetTestFilterForShard(tests, mShardIndex, mShardCount);

        // Note that we only add a filter string if we previously deleted a shader index/count
        // argument. So we will have space for the new filter string in argv.
        AddArg(argc, argv, mFilterString.c_str());
    }

    if (mResultsDirectory)
    {
        testing::TestEventListeners &listeners = testing::UnitTest::GetInstance()->listeners();
        listeners.Append(new TestEventListener(mResultsDirectory, mTestSuiteName.c_str()));
    }

    testing::InitGoogleTest(argc, argv);
}

TestSuite::~TestSuite() = default;

void TestSuite::parseTestSuiteName(const char *executable)
{
    const char *baseNameStart = strrchr(executable, kPathSeparator);
    if (!baseNameStart)
    {
        baseNameStart = executable;
    }
    else
    {
        baseNameStart++;
    }

    const char kSuffix[]       = ".exe";
    const char *baseNameSuffix = strstr(baseNameStart, kSuffix);
    if (baseNameSuffix == (baseNameStart + strlen(baseNameStart) - strlen(kSuffix)))
    {
        mTestSuiteName.insert(mTestSuiteName.begin(), baseNameStart, baseNameSuffix);
    }
    else
    {
        mTestSuiteName = baseNameStart;
    }
}

bool TestSuite::parseTestSuiteFlag(const char *argument)
{
    return (ParseIntFlag("--shard-count=", argument, &mShardCount) ||
            ParseIntFlag("--shard-index=", argument, &mShardIndex) ||
            ParseStringFlag("--results-directory=", argument, &mResultsDirectory));
}
}  // namespace tr
