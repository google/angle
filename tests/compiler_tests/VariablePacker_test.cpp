//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#include "gtest/gtest.h"
#include "angle_gl.h"
#include "common/utilities.h"
#include "common/angleutils.h"
#include "compiler/translator/VariablePacker.h"

static sh::GLenum types[] = {
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

static sh::GLenum nonSqMatTypes[] = {
  GL_FLOAT_MAT2x3,
  GL_FLOAT_MAT2x4,
  GL_FLOAT_MAT3x2,
  GL_FLOAT_MAT3x4,
  GL_FLOAT_MAT4x2,
  GL_FLOAT_MAT4x3
};

TEST(VariablePacking, Pack) {
  VariablePacker packer;
  TVariableInfoList vars;
  const int kMaxRows = 16;
  // test no vars.
  EXPECT_TRUE(packer.CheckVariablesWithinPackingLimits(kMaxRows, vars));

  for (size_t tt = 0; tt < ArraySize(types); ++tt) {
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

TEST(VariablePacking, PackSizes) {
  for (size_t tt = 0; tt < ArraySize(types); ++tt) {
    GLenum type = types[tt];

    int expectedComponents = gl::VariableComponentCount(type);
    int expectedRows = gl::VariableRowCount(type);

    if (type == GL_FLOAT_MAT2) {
      expectedComponents = 4;
    } else if (gl::IsMatrixType(type)) {
      int squareSize = std::max(gl::VariableRowCount(type),
          gl::VariableColumnCount(type));
      expectedComponents = squareSize;
      expectedRows = squareSize;
    }

    EXPECT_EQ(expectedComponents,
      VariablePacker::GetNumComponentsPerRow(type));
    EXPECT_EQ(expectedRows, VariablePacker::GetNumRows(type));
  }
}

// Check special assumptions about packing non-square mats
TEST(VariablePacking, NonSquareMats) {

  for (size_t mt = 0; mt < ArraySize(nonSqMatTypes); ++mt) {
    
    GLenum type = nonSqMatTypes[mt];

    int rows = gl::VariableRowCount(type);
    int cols = gl::VariableColumnCount(type);
    int squareSize = std::max(rows, cols);

    TVariableInfoList vars;
    vars.push_back(TVariableInfo(type, 1));

    // Fill columns
    for (int row = 0; row < squareSize; row++) {
      for (int col = squareSize; col < 4; ++col) {
        vars.push_back(TVariableInfo(GL_FLOAT, 1));
      }
    }

    VariablePacker packer;

    EXPECT_TRUE(packer.CheckVariablesWithinPackingLimits(squareSize, vars));

    // and one scalar and packing should fail
    vars.push_back(TVariableInfo(GL_FLOAT, 1));
    EXPECT_FALSE(packer.CheckVariablesWithinPackingLimits(squareSize, vars));
  }
}
