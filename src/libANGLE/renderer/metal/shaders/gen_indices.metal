//
// Copyright 2019 The ANGLE Project. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "common.h"

struct IndexConversionParams
{
    uint32_t srcOffset;  // offset in bytes
    uint32_t indexCount;
};

#define ANGLE_IDX_CONVERSION_GUARD(IDX, OPTS) \
    if (IDX >= OPTS.indexCount)               \
    {                                         \
        return;                               \
    }

kernel void convertIndexU8ToU16(uint idx[[thread_position_in_grid]],
                                constant IndexConversionParams &options[[buffer(0)]],
                                constant uchar *input[[buffer(1)]],
                                device ushort *output[[buffer(2)]])
{
    ANGLE_IDX_CONVERSION_GUARD(idx, options);
    output[idx] = input[options.srcOffset + idx];
}

kernel void convertIndexU16Unaligned(uint idx[[thread_position_in_grid]],
                                     constant IndexConversionParams &options[[buffer(0)]],
                                     constant uchar *input[[buffer(1)]],
                                     device ushort *output[[buffer(2)]])
{
    ANGLE_IDX_CONVERSION_GUARD(idx, options);
    ushort inputLo = input[options.srcOffset + 2 * idx];
    ushort inputHi = input[options.srcOffset + 2 * idx + 1];
    // Little endian conversion:
    ushort value = inputLo | (inputHi << 8);
    output[idx] = value;
}

kernel void convertIndexU16Aligned(uint idx[[thread_position_in_grid]],
                                   constant IndexConversionParams &options[[buffer(0)]],
                                   constant ushort *input[[buffer(1)]],
                                   device ushort *output[[buffer(2)]])
{
    ANGLE_IDX_CONVERSION_GUARD(idx, options);
    ushort value = input[options.srcOffset / 2 + idx];
    output[idx] = value;
}

kernel void convertIndexU32Unaligned(uint idx[[thread_position_in_grid]],
                                     constant IndexConversionParams &options[[buffer(0)]],
                                     constant uchar *input[[buffer(1)]],
                                     device uint *output[[buffer(2)]])
{
    ANGLE_IDX_CONVERSION_GUARD(idx, options);
    uint input0 = input[options.srcOffset + 4 * idx];
    uint input1 = input[options.srcOffset + 4 * idx + 1];
    uint input2 = input[options.srcOffset + 4 * idx + 2];
    uint input3 = input[options.srcOffset + 4 * idx + 3];
    // Little endian conversion:
    uint value = input0 | (input1 << 8) | (input2 << 16) | (input3 << 24);
    output[idx] = value;
}

kernel void convertIndexU32Aligned(uint idx[[thread_position_in_grid]],
                                   constant IndexConversionParams &options[[buffer(0)]],
                                   constant uint *input[[buffer(1)]],
                                   device uint *output[[buffer(2)]])
{
    ANGLE_IDX_CONVERSION_GUARD(idx, options);
    uint value = input[options.srcOffset / 4 + idx];
    output[idx] = value;
}
