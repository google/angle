//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// MemoryProgramCache: Stores compiled and linked programs in memory so they don't
//   always have to be re-compiled. Can be used in conjunction with the platform
//   layer to warm up the cache from disk.

#ifndef LIBANGLE_MEMORY_PROGRAM_CACHE_H_
#define LIBANGLE_MEMORY_PROGRAM_CACHE_H_

#include "common/MemoryBuffer.h"
#include "libANGLE/Error.h"

namespace gl
{
class Context;
class InfoLog;
class Program;
class ProgramState;

class MemoryProgramCache final : angle::NonCopyable
{
  public:
    // Writes a program's binary to the output memory buffer.
    static void Serialize(const Context *context,
                          const Program *program,
                          angle::MemoryBuffer *binaryOut);

    // Loads program state according to the specified binary blob.
    static LinkResult Deserialize(const Context *context,
                                  const Program *program,
                                  ProgramState *state,
                                  const uint8_t *binary,
                                  size_t length,
                                  InfoLog &infoLog);
};

}  // namespace gl

#endif  // LIBANGLE_MEMORY_PROGRAM_CACHE_H_
