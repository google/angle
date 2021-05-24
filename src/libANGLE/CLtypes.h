//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLtypes.h: Defines common types for the OpenCL support in ANGLE.

#ifndef LIBANGLE_CLTYPES_H_
#define LIBANGLE_CLTYPES_H_

#include "libANGLE/CLRefPointer.h"

#include "common/PackedCLEnums_autogen.h"

// Include frequently used standard headers
#include <algorithm>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace cl
{

class Buffer;
class CommandQueue;
class Context;
class Device;
class Event;
class Image;
class Kernel;
class Memory;
class Object;
class Platform;
class Program;
class Sampler;

using CommandQueuePtr = std::unique_ptr<CommandQueue>;
using ContextPtr      = std::unique_ptr<Context>;
using DevicePtr       = std::unique_ptr<Device>;
using EventPtr        = std::unique_ptr<Event>;
using KernelPtr       = std::unique_ptr<Kernel>;
using MemoryPtr       = std::unique_ptr<Memory>;
using ObjectPtr       = std::unique_ptr<Object>;
using PlatformPtr     = std::unique_ptr<Platform>;
using ProgramPtr      = std::unique_ptr<Program>;
using SamplerPtr      = std::unique_ptr<Sampler>;

using ContextRefPtr = RefPointer<Context>;
using DeviceRefPtr  = RefPointer<Device>;
using MemoryRefPtr  = RefPointer<Memory>;

using DevicePtrList = std::list<DevicePtr>;
using DeviceRefList = std::vector<DeviceRefPtr>;

using Binary   = std::vector<unsigned char>;
using Binaries = std::vector<Binary>;

struct ImageDescriptor
{
    cl_mem_object_type type;
    size_t width;
    size_t height;
    size_t depth;
    size_t arraySize;
    size_t rowPitch;
    size_t slicePitch;
    cl_uint numMipLevels;
    cl_uint numSamples;
};

}  // namespace cl

#endif  // LIBANGLE_CLTYPES_H_
