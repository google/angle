//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// crash_handler_posix:
//    ANGLE's crash handling and stack walking code. Modified from Skia's:
//     https://github.com/google/skia/blob/master/tools/CrashHandler.cpp
//

#include "util/test_utils.h"

#include "common/FixedVector.h"
#include "common/angleutils.h"
#include "common/system_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>

#if !defined(ANGLE_PLATFORM_ANDROID) && !defined(ANGLE_PLATFORM_FUCHSIA)
#    if defined(ANGLE_PLATFORM_APPLE)
// We only use local unwinding, so we can define this to select a faster implementation.
#        define UNW_LOCAL_ONLY
#        include <cxxabi.h>
#        include <libunwind.h>
#        include <signal.h>
#    elif defined(ANGLE_PLATFORM_POSIX)
// We'd use libunwind here too, but it's a pain to get installed for
// both 32 and 64 bit on bots.  Doesn't matter much: catchsegv is best anyway.
#        include <cxxabi.h>
#        include <dlfcn.h>
#        include <execinfo.h>
#        include <libgen.h>
#        include <signal.h>
#        include <string.h>
#    endif  // defined(ANGLE_PLATFORM_APPLE)
#endif      // !defined(ANGLE_PLATFORM_ANDROID) && !defined(ANGLE_PLATFORM_FUCHSIA)

namespace angle
{
#if defined(ANGLE_PLATFORM_ANDROID) || defined(ANGLE_PLATFORM_FUCHSIA)

void PrintStackBacktrace()
{
    // No implementations yet.
}

void InitCrashHandler(CrashCallback *callback)
{
    // No implementations yet.
}

void TerminateCrashHandler()
{
    // No implementations yet.
}

#else
namespace
{
CrashCallback *gCrashHandlerCallback;
}  // namespace

#    if defined(ANGLE_PLATFORM_APPLE)

void PrintStackBacktrace()
{
    printf("Backtrace:\n");

    unw_context_t context;
    unw_getcontext(&context);

    unw_cursor_t cursor;
    unw_init_local(&cursor, &context);

    while (unw_step(&cursor) > 0)
    {
        static const size_t kMax = 256;
        char mangled[kMax], demangled[kMax];
        unw_word_t offset;
        unw_get_proc_name(&cursor, mangled, kMax, &offset);

        int ok;
        size_t len = kMax;
        abi::__cxa_demangle(mangled, demangled, &len, &ok);

        printf("    %s (+0x%zx)\n", ok == 0 ? demangled : mangled, (size_t)offset);
    }
    printf("\n");
}

static void Handler(int sig)
{
    if (gCrashHandlerCallback)
    {
        (*gCrashHandlerCallback)();
    }

    printf("\nSignal %d:\n", sig);
    PrintStackBacktrace();

    // Exit NOW.  Don't notify other threads, don't call anything registered with atexit().
    _Exit(sig);
}

#    elif defined(ANGLE_PLATFORM_POSIX)

// Can control this at a higher level if required.
#        define ANGLE_HAS_ADDR2LINE

#        if defined(ANGLE_HAS_ADDR2LINE)
namespace
{
constexpr size_t kAddr2LineMaxParameters = 50;
using Addr2LineCommandLine = angle::FixedVector<const char *, kAddr2LineMaxParameters>;
void CallAddr2Line(const Addr2LineCommandLine &commandLine)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        std::cerr << "Error: Failed to fork()" << std::endl;
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
        // Ignore the status, since we aren't going to handle it anyway.
    }
    else
    {
        // Child process executes addr2line
        //
        // See comment in test_utils_posix.cpp::PosixProcess regarding const_cast.
        execv(commandLine[0], const_cast<char *const *>(commandLine.data()));
        std::cerr << "Error: Child process returned from exevc()" << std::endl;
        _exit(EXIT_FAILURE);  // exec never returns
    }
}
}  // anonymous namespace
#        endif  // defined(ANGLE_HAS_ADDR2LINE)

void PrintStackBacktrace()
{
    printf("Backtrace:\n");

    void *stack[64];
    const int count = backtrace(stack, ArraySize(stack));
    char **symbols  = backtrace_symbols(stack, count);

#        if defined(ANGLE_HAS_ADDR2LINE)
    // Child process executes addr2line
    constexpr size_t kAddr2LineFixedParametersCount = 6;
    Addr2LineCommandLine commandLineArgs            = {
        "/usr/bin/addr2line",  // execv requires an absolute path to find addr2line
        "-s",
        "-p",
        "-f",
        "-C",
        "-e",
    };
    const char *currentModule = "";
    std::string resolvedModule;

    for (int i = 0; i < count; i++)
    {
        char *symbol = symbols[i];

        // symbol looks like the following:
        //
        //     path/to/module(+address) [globalAddress]
        //
        // We are interested in module and address.  symbols[i] is modified in place to replace '('
        // and ')' with 0, allowing c-strings to point to the module and address.  This allows
        // accumulating addresses without having to create another storage for them.
        //
        // If module is not an absolute path, it needs to be resolved.

        char *module  = symbol;
        char *address = strchr(symbol, '+') + 1;

        *strchr(module, '(')  = 0;
        *strchr(address, ')') = 0;

        // If module is the same as last, continue batching addresses.  If commandLineArgs has
        // reached its capacity however, make the call to addr2line already.  Note that there should
        // be one entry left for the terminating nullptr at the end of the command line args.
        if (strcmp(module, currentModule) == 0 &&
            commandLineArgs.size() + 1 < commandLineArgs.max_size())
        {
            commandLineArgs.push_back(address);
            continue;
        }

        // If there's a command batched, execute it before modifying currentModule (a pointer to
        // which is stored in the command line args).
        if (currentModule[0] != 0)
        {
            commandLineArgs.push_back(nullptr);
            CallAddr2Line(commandLineArgs);
        }

        // Reset the command line and remember this module as the current.
        resolvedModule = currentModule = module;
        commandLineArgs.resize(kAddr2LineFixedParametersCount);

        // We need an absolute path to get to the executable and all of the various shared objects,
        // but the caller may have used a relative path to launch the executable, so build one up if
        // we don't see a leading '/'.
        if (resolvedModule.at(0) != GetPathSeparator())
        {
            const Optional<std::string> &cwd = angle::GetCWD();
            if (!cwd.valid())
            {
                std::cerr << "Error getting CWD to print the backtrace." << std::endl;
            }
            else
            {
                std::string absolutePath = cwd.value();
                size_t lastPathSepLoc    = resolvedModule.find_last_of(GetPathSeparator());
                std::string relativePath = resolvedModule.substr(0, lastPathSepLoc);

                // Remove "." from the relativePath path
                // For example: ./out/LinuxDebug/angle_perftests
                size_t pos = relativePath.find('.');
                if (pos != std::string::npos)
                {
                    // If found then erase it from string
                    relativePath.erase(pos, 1);
                }

                // Remove the overlapping relative path from the CWD so we can build the full
                // absolute path.
                // For example:
                // absolutePath = /home/timvp/code/angle/out/LinuxDebug
                // relativePath = /out/LinuxDebug
                pos = absolutePath.find(relativePath);
                if (pos != std::string::npos)
                {
                    // If found then erase it from string
                    absolutePath.erase(pos, relativePath.length());
                }
                resolvedModule = absolutePath + GetPathSeparator() + resolvedModule;
            }
        }

        commandLineArgs.push_back(resolvedModule.c_str());
        commandLineArgs.push_back(address);
    }

    // Call addr2line for the last batch of addresses.
    if (currentModule[0] != 0)
    {
        commandLineArgs.push_back(nullptr);
        CallAddr2Line(commandLineArgs);
    }
#        else
    for (int i = 0; i < count; i++)
    {
        Dl_info info;
        if (dladdr(stack[i], &info) && info.dli_sname)
        {
            // Make sure this is large enough to hold the fully demangled names, otherwise we could
            // segault/hang here. For example, Vulkan validation layer errors can be deep enough
            // into the stack that very large symbol names are generated.
            char demangled[4096];
            size_t len = ArraySize(demangled);
            int ok;

            abi::__cxa_demangle(info.dli_sname, demangled, &len, &ok);
            if (ok == 0)
            {
                printf("    %s\n", demangled);
                continue;
            }
        }
        printf("    %s\n", symbols[i]);
    }
#        endif  // defined(ANGLE_HAS_ADDR2LINE)
}

static void Handler(int sig)
{
    if (gCrashHandlerCallback)
    {
        (*gCrashHandlerCallback)();
    }

    printf("\nSignal %d [%s]:\n", sig, strsignal(sig));
    PrintStackBacktrace();

    // Exit NOW.  Don't notify other threads, don't call anything registered with atexit().
    _Exit(sig);
}

#    endif  // defined(ANGLE_PLATFORM_APPLE)

static constexpr int kSignals[] = {
    SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGSEGV, SIGTRAP,
};

void InitCrashHandler(CrashCallback *callback)
{
    gCrashHandlerCallback = callback;
    for (int sig : kSignals)
    {
        // Register our signal handler unless something's already done so (e.g. catchsegv).
        void (*prev)(int) = signal(sig, Handler);
        if (prev != SIG_DFL)
        {
            signal(sig, prev);
        }
    }
}

void TerminateCrashHandler()
{
    gCrashHandlerCallback = nullptr;
    for (int sig : kSignals)
    {
        void (*prev)(int) = signal(sig, SIG_DFL);
        if (prev != Handler && prev != SIG_DFL)
        {
            signal(sig, prev);
        }
    }
}

#endif  // defined(ANGLE_PLATFORM_ANDROID) || defined(ANGLE_PLATFORM_FUCHSIA)

}  // namespace angle
