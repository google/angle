//
// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_WGSL_OUTPUT_UNIFORM_BLOCKS_H_
#define COMPILER_TRANSLATOR_WGSL_OUTPUT_UNIFORM_BLOCKS_H_

#include "compiler/translator/Compiler.h"
#include "compiler/translator/IntermNode.h"

namespace sh
{

const char kDefaultUniformBlockVarType[]     = "ANGLE_DefaultUniformBlock";
const char kDefaultUniformBlockVarName[]     = "ANGLE_defaultUniformBlock";
const uint32_t kDefaultUniformBlockBindGroup = 0;
const uint32_t kDefaultVertexUniformBlockBinding   = 0;
const uint32_t kDefaultFragmentUniformBlockBinding = 1;

struct UniformBlockMetadata
{
    // A list of structs used anywhere in the uniform address space. These will require special
    // handling (@align() attributes, wrapping of basic types, etc.) to ensure they fit WGSL's
    // uniform layout requirements.
    // The key is TSymbolUniqueId::get().
    TUnorderedSet<int> structsInUniformAddressSpace;
};

// Given a GLSL AST `root`, fills in `outMetadata`, to be used when outputting WGSL.
// If the AST is manipulated after calling this, it may be out of sync with the data recorded in
// `outMetadata`.
bool RecordUniformBlockMetadata(TIntermBlock *root, UniformBlockMetadata &outMetadata);

// TODO(anglebug.com/42267100): for now does not output all uniform blocks,
// just the default block. (fails for  matCx2, bool, and arrays with stride less than 16.)
bool OutputUniformBlocks(TCompiler *compiler, TIntermBlock *root);

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_WGSL_OUTPUT_UNIFORM_BLOCKS_H_
