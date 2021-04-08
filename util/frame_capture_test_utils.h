//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// frame_capture_test_utils:
//   Helper functions for capture and replay of traces.
//

#ifndef UTIL_FRAME_CAPTURE_TEST_UTILS_H_
#define UTIL_FRAME_CAPTURE_TEST_UTILS_H_

#include <iostream>
#include <memory>
#include <sstream>
#include <type_traits>
#include <vector>

#include "common/angleutils.h"
#include "common/system_utils.h"

#define USE_SYSTEM_ZLIB
#include "compression_utils_portable.h"

#define ANGLE_MACRO_STRINGIZE_AUX(a) #a
#define ANGLE_MACRO_STRINGIZE(a) ANGLE_MACRO_STRINGIZE_AUX(a)
#define ANGLE_MACRO_CONCAT_AUX(a, b) a##b
#define ANGLE_MACRO_CONCAT(a, b) ANGLE_MACRO_CONCAT_AUX(a, b)

namespace angle
{

inline uint8_t *DecompressBinaryData(const std::vector<uint8_t> &compressedData)
{
    uint32_t uncompressedSize =
        zlib_internal::GetGzipUncompressedSize(compressedData.data(), compressedData.size());

    std::unique_ptr<uint8_t[]> uncompressedData(new uint8_t[uncompressedSize]);
    uLong destLen = uncompressedSize;
    int zResult =
        zlib_internal::GzipUncompressHelper(uncompressedData.get(), &destLen, compressedData.data(),
                                            static_cast<uLong>(compressedData.size()));

    if (zResult != Z_OK)
    {
        std::cerr << "Failure to decompressed binary data: " << zResult << "\n";
        return nullptr;
    }

    return uncompressedData.release();
}

using DecompressCallback = uint8_t *(*)(const std::vector<uint8_t> &);

using SetBinaryDataDecompressCallbackFunc = void (*)(DecompressCallback);
using SetBinaryDataDirFunc                = void (*)(const char *);
using SetupReplayFunc                     = void (*)();
using ReplayFrameFunc                     = void (*)(uint32_t);
using ResetReplayFunc                     = void (*)();
using FinishReplayFunc                    = void (*)();
using GetSerializedContextStateFunc       = const char *(*)(uint32_t);

class TraceLibrary
{
  public:
    TraceLibrary(const char *traceNameIn)
    {
        std::stringstream traceNameStr;
#if !defined(ANGLE_PLATFORM_WINDOWS)
        traceNameStr << "lib";
#endif  // !defined(ANGLE_PLATFORM_WINDOWS)
        traceNameStr << traceNameIn;
#if defined(ANGLE_PLATFORM_ANDROID) && defined(COMPONENT_BUILD)
        // Added to shared library names in Android component builds in
        // https://chromium.googlesource.com/chromium/src/+/9bacc8c4868cc802f69e1e858eea6757217a508f/build/toolchain/toolchain.gni#56
        traceNameStr << ".cr";
#endif  // defined(ANGLE_PLATFORM_ANDROID) && defined(COMPONENT_BUILD)
        std::string traceName = traceNameStr.str();
        mTraceLibrary.reset(OpenSharedLibrary(traceName.c_str(), SearchType::ApplicationDir));
    }

    bool valid() const { return mTraceLibrary != nullptr; }

    void setBinaryDataDir(const char *dataDir)
    {
        callFunc<SetBinaryDataDirFunc>("SetBinaryDataDir", dataDir);
    }

    void setBinaryDataDecompressCallback(DecompressCallback callback)
    {
        callFunc<SetBinaryDataDecompressCallbackFunc>("SetBinaryDataDecompressCallback", callback);
    }

    void replayFrame(uint32_t frameIndex) { callFunc<ReplayFrameFunc>("ReplayFrame", frameIndex); }

    void setupReplay() { callFunc<SetupReplayFunc>("SetupReplay"); }

    void resetReplay() { callFunc<ResetReplayFunc>("ResetReplay"); }

    void finishReplay() { callFunc<FinishReplayFunc>("FinishReplay"); }

    const char *getSerializedContextState(uint32_t frameIndex)
    {
        return callFunc<GetSerializedContextStateFunc>("GetSerializedContextState", frameIndex);
    }

  private:
    template <typename FuncT, typename... ArgsT>
    typename std::result_of<FuncT(ArgsT...)>::type callFunc(const char *funcName, ArgsT... args)
    {
        void *untypedFunc = mTraceLibrary->getSymbol(funcName);
        if (!untypedFunc)
        {
            fprintf(stderr, "Error loading function: %s\n", funcName);
            ASSERT(untypedFunc);
        }
        auto typedFunc = reinterpret_cast<FuncT>(untypedFunc);
        return typedFunc(args...);
    }

    std::unique_ptr<Library> mTraceLibrary;
};

}  // namespace angle

#endif  // UTIL_FRAME_CAPTURE_TEST_UTILS_H_
