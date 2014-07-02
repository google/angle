//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#include "gtest/gtest.h"
#include "angle_gl.h"
#include "compiler/translator/VariablePacker.h"

TEST(VariablePacking, Pack) {
  VariablePacker packer;
  TVariableInfoList vars;
  const int kMaxRows = 16;
  // test no vars.
  EXPECT_TRUE(packer.CheckVariablesWithinPackingLimits(kMaxRows, vars));

  sh::GLenum types[] = {
    GL_FLOAT_MAT4,            // 0
    GL_FLOAT_MAT2,            // 1
    GL_FLOAT_VEC4,            // 2
    GL_INT_VEC4,              // 3
    GL_BOOL_VEC4,             // 4
    GL_FLOAT_MAT3,            // 5
    GL_FLOAT_VEC3,            // 6
    GL_INT_VEC3,              // 7
    GL_BOOL_VEC3,             // 8
    GL_FLOAT_VEC2,            // 9
    GL_INT_VEC2,              // 10
    GL_BOOL_VEC2,             // 11
    GL_FLOAT,                 // 12
    GL_INT,                   // 13
    GL_BOOL,                  // 14
    GL_SAMPLER_2D,            // 15
    GL_SAMPLER_CUBE,          // 16
    GL_SAMPLER_EXTERNAL_OES,  // 17
    GL_SAMPLER_2D_RECT_ARB,   // 18
  };

  for (size_t tt = 0; tt < sizeof(types) / sizeof(types[0]); ++tt) {
    sh::GLenum type = types[tt];
    int num_rows = VariablePacker::GetNumRows(type);
    int num_components_per_row = VariablePacker::GetNumComponentsPerRow(type);
    // Check 1 of the type.
    vars.clear();
    vars.push_back(TVariableInfo(type, 1));
    EXPECT_TRUE(packer.CheckVariablesWithinPackingLimits(kMaxRows, vars));

    // Check exactly the right amount of 1 type as an array.
    int num_vars = kMaxRows / num_rows;
    vars.clear();
    vars.push_back(TVariableInfo(type, num_vars));
    EXPECT_TRUE(packer.CheckVariablesWithinPackingLimits(kMaxRows, vars));

    // test too many
    vars.clear();
    vars.push_back(TVariableInfo(type, num_vars + 1));
    EXPECT_FALSE(packer.CheckVariablesWithinPackingLimits(kMaxRows, vars));

    // Check exactly the right amount of 1 type as individual vars.
    num_vars = kMaxRows / num_rows *
        ((num_components_per_row > 2) ? 1 : (4 / num_components_per_row));
    vars.clear();
    for (int ii = 0; ii < num_vars; ++ii) {
      vars.push_back(TVariableInfo(type, 1));
    }
    EXPECT_TRUE(packer.CheckVariablesWithinPackingLimits(kMaxRows, vars));

    // Check 1 too many.
    vars.push_back(TVariableInfo( type, 1));
    EXPECT_FALSE(packer.CheckVariablesWithinPackingLimits(kMaxRows, vars));
  }

  // Test example from GLSL ES 3.0 spec chapter 11.
  vars.clear();
  vars.push_back(TVariableInfo(GL_FLOAT_VEC4, 1));
  vars.push_back(TVariableInfo(GL_FLOAT_MAT3, 1));
  vars.push_back(TVariableInfo(GL_FLOAT_MAT3, 1));
  vars.push_back(TVariableInfo(GL_FLOAT_VEC2, 6));
  vars.push_back(TVariableInfo(GL_FLOAT_VEC2, 4));
  vars.push_back(TVariableInfo(GL_FLOAT_VEC2, 1));
  vars.push_back(TVariableInfo(GL_FLOAT, 3));
  vars.push_back(TVariableInfo(GL_FLOAT, 2));
  vars.push_back(TVariableInfo(GL_FLOAT, 1));
  EXPECT_TRUE(packer.CheckVariablesWithinPackingLimits(kMaxRows, vars));
}

