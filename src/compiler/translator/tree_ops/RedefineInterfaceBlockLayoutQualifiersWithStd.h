//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RedefineInterfaceBlockLayoutQualifiersWithStd: Change the memory layout qualifier of interface
// blocks if not specifically requested to be std140 or std430, i.e. the memory layout qualifier is
// changed if it's unspecified, shared or packed.  This makes the layout qualifiers conformant with
// Vulkan GLSL (GL_KHR_vulkan_glsl).
//
// - For uniform buffers, std140 is used.  It would have been more efficient to default to std430,
//   but that would require GL_EXT_scalar_block_layout.
// - For storage buffers, std430 is used.

#ifndef COMPILER_TRANSLATOR_TREEOPS_REDEFINEINTERFACEBLOCKLAYOUTQUALIFIERSWITHSTD_H_
#define COMPILER_TRANSLATOR_TREEOPS_REDEFINEINTERFACEBLOCKLAYOUTQUALIFIERSWITHSTD_H_

namespace sh
{
class TIntermBlock;
class TSymbolTable;
void RedefineInterfaceBlockLayoutQualifiersWithStd(TIntermBlock *root, TSymbolTable *symbolTable);
}  // namespace sh

#endif  // COMPILER_TRANSLATOR_TREEOPS_REDEFINEINTERFACEBLOCKLAYOUTQUALIFIERSWITHSTD_H_
