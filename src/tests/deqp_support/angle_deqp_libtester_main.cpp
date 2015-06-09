//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// deqp_libtester_main.cpp: Entry point for tester DLL.

#include <cstdio>
#include <iostream>

#include "angle_deqp_libtester.h"
#include "deMath.h"
#include "deUniquePtr.hpp"
#include "tcuApp.hpp"
#include "tcuCommandLine.hpp"
#include "tcuDefs.hpp"
#include "tcuPlatform.hpp"
#include "tcuRandomOrderExecutor.h"
#include "tcuResource.hpp"
#include "tcuTestLog.hpp"

// Located in tcuMain.cc in dEQP's sources.
int main(int argc, const char* argv[]);
tcu::Platform *createPlatform();

namespace
{

tcu::Platform *g_platform = nullptr;
tcu::CommandLine *g_cmdLine = nullptr;
tcu::DirArchive *g_archive = nullptr;
tcu::TestLog *g_log = nullptr;
tcu::TestContext *g_testCtx = nullptr;
tcu::TestPackageRoot *g_root = nullptr;
tcu::RandomOrderExecutor *g_executor = nullptr;

}

// Exported to the tester app.
ANGLE_LIBTESTER_EXPORT int deqp_libtester_main(int argc, const char *argv[])
{
    return main(argc, argv);
}

ANGLE_LIBTESTER_EXPORT void deqp_libtester_init_platform(int argc, char **argv, const char *deqpDataDir)
{
    try
    {
#if (DE_OS != DE_OS_WIN32)
        // Set stdout to line-buffered mode (will be fully buffered by default if stdout is pipe).
        setvbuf(stdout, DE_NULL, _IOLBF, 4 * 1024);
#endif
        g_platform = createPlatform();

        if (!deSetRoundingMode(DE_ROUNDINGMODE_TO_NEAREST))
        {
            throw std::runtime_error("Failed to set floating point rounding mode.");
        }

        // TODO(jmadill): filter arguments
        g_cmdLine = new tcu::CommandLine(1, argv);
        g_archive = new tcu::DirArchive(deqpDataDir);
        g_log = new tcu::TestLog(g_cmdLine->getLogFileName(), g_cmdLine->getLogFlags());
        g_testCtx = new tcu::TestContext(*g_platform, *g_archive, *g_log, *g_cmdLine, DE_NULL);
        g_root = new tcu::TestPackageRoot(*g_testCtx, tcu::TestPackageRegistry::getSingleton());
        g_executor = new tcu::RandomOrderExecutor(*g_root, *g_testCtx);
    }
    catch (const std::exception& e)
    {
        tcu::die("%s", e.what());
    }
}

ANGLE_LIBTESTER_EXPORT void deqp_libtester_shutdown_platform()
{
    delete g_executor;
    delete g_root;
    delete g_testCtx;
    delete g_log;
    delete g_archive;
    delete g_cmdLine;
    delete g_platform;
}

ANGLE_LIBTESTER_EXPORT bool deqp_libtester_run(const char *caseName)
{
    if (!g_platform)
    {
        return false;
    }

    try
    {
        // Poll platform events
        const bool platformOk = g_platform->processEvents();

        if (platformOk)
        {
            const tcu::TestStatus &result = g_executor->execute(caseName);
            switch (result.getCode())
            {
                case QP_TEST_RESULT_PASS:
                case QP_TEST_RESULT_NOT_SUPPORTED:
                    return true;
                case QP_TEST_RESULT_QUALITY_WARNING:
                    std::cout << "Quality warning! " << result.getDescription() << std::endl;
                    return true;
                case QP_TEST_RESULT_COMPATIBILITY_WARNING:
                    std::cout << "Compatiblity warning! " << result.getDescription() << std::endl;
                    return true;
                default:
                    return false;
            }
        }
        else
        {
            std::cout << "Aborted test!" << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception running test: " << e.what() << std::endl;
    }

    return false;
}
