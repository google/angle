//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// VaryingPacking_unittest.cpp:
//   Tests for ANGLE's internal varying packing algorithm.
//

#include <gtest/gtest.h>
// 'None' is defined as 'struct None {};' in
// third_party/googletest/src/googletest/include/gtest/internal/gtest-type-util.h.
// But 'None' is also define as a numberic constant 0L in <X11/X.h>.
// So we need to include gtest first to avoid such conflict.

#include "libANGLE/Program.h"
#include "libANGLE/VaryingPacking.h"

using namespace gl;

namespace
{

class VaryingPackingTest : public ::testing::TestWithParam<GLuint>
{
  protected:
    VaryingPackingTest() {}

    bool testVaryingPacking(GLint maxVaryings,
                            PackMode packMode,
                            const std::vector<sh::ShaderVariable> &shVaryings)
    {
        ProgramMergedVaryings mergedVaryings;
        for (const sh::ShaderVariable &shVarying : shVaryings)
        {
            ProgramVaryingRef ref;
            ref.frontShader      = &shVarying;
            ref.backShader       = &shVarying;
            ref.frontShaderStage = ShaderType::Vertex;
            ref.backShaderStage  = ShaderType::Fragment;
            mergedVaryings.push_back(ref);
        }

        InfoLog infoLog;
        std::vector<std::string> transformFeedbackVaryings;

        VaryingPacking varyingPacking;
        return varyingPacking.collectAndPackUserVaryings(
            infoLog, maxVaryings, packMode, ShaderType::Vertex, ShaderType::Fragment,
            mergedVaryings, transformFeedbackVaryings, false);
    }

    bool testVaryingPackingWithTransformFeedback(
        GLint maxVaryings,
        PackMode packMode,
        const std::vector<sh::ShaderVariable> &vertexVaryings,
        const std::vector<std::string> &transformFeedbackVaryings)
    {
        ProgramMergedVaryings mergedVaryings;
        for (const sh::ShaderVariable &shVarying : vertexVaryings)
        {
            ProgramVaryingRef ref;
            ref.frontShader      = &shVarying;
            ref.backShader       = nullptr;
            ref.frontShaderStage = ShaderType::Vertex;
            ref.backShaderStage  = ShaderType::Fragment;
            mergedVaryings.push_back(ref);
        }

        InfoLog infoLog;
        VaryingPacking varyingPacking;
        return varyingPacking.collectAndPackUserVaryings(
            infoLog, maxVaryings, packMode, ShaderType::Vertex, ShaderType::Fragment,
            mergedVaryings, transformFeedbackVaryings, false);
    }

    // Uses the "relaxed" ANGLE packing mode.
    bool packVaryings(GLint maxVaryings, const std::vector<sh::ShaderVariable> &shVaryings)
    {
        return testVaryingPacking(maxVaryings, PackMode::ANGLE_RELAXED, shVaryings);
    }

    // Uses the stricter WebGL style packing rules.
    bool packVaryingsStrict(GLint maxVaryings, const std::vector<sh::ShaderVariable> &shVaryings)
    {
        return testVaryingPacking(maxVaryings, PackMode::WEBGL_STRICT, shVaryings);
    }

    const int kMaxVaryings = GetParam();
};

std::vector<sh::ShaderVariable> MakeVaryings(GLenum type, size_t count, size_t arraySize)
{
    std::vector<sh::ShaderVariable> varyings;

    for (size_t index = 0; index < count; ++index)
    {
        std::stringstream strstr;
        strstr << type << index;

        sh::ShaderVariable varying;
        varying.type       = type;
        varying.precision  = GL_MEDIUM_FLOAT;
        varying.name       = strstr.str();
        varying.mappedName = strstr.str();
        if (arraySize > 0)
        {
            varying.arraySizes.push_back(static_cast<unsigned int>(arraySize));
        }
        varying.staticUse     = true;
        varying.interpolation = sh::INTERPOLATION_FLAT;
        varying.isInvariant   = false;

        varyings.push_back(varying);
    }

    return varyings;
}

sh::ShaderVariable MakeStructVarying(const std::string &name, GLenum fieldType, size_t fieldCount)
{
    sh::ShaderVariable varying;
    varying.type              = GL_NONE;
    varying.precision         = GL_NONE;
    varying.name              = name;
    varying.mappedName        = name;
    varying.structOrBlockName = "S";
    varying.staticUse         = true;
    varying.active            = true;
    varying.interpolation     = sh::INTERPOLATION_FLAT;
    varying.isInvariant       = false;

    for (size_t index = 0; index < fieldCount; ++index)
    {
        std::stringstream fieldName;
        fieldName << "m" << index;

        sh::ShaderVariable field;
        field.type          = fieldType;
        field.precision     = GL_MEDIUM_FLOAT;
        field.name          = fieldName.str();
        field.mappedName    = fieldName.str();
        field.staticUse     = true;
        field.active        = true;
        field.interpolation = sh::INTERPOLATION_FLAT;
        field.isInvariant   = false;

        varying.fields.push_back(field);
    }

    return varying;
}

void AddVaryings(std::vector<sh::ShaderVariable> *varyings,
                 GLenum type,
                 size_t count,
                 size_t arraySize)
{
    const auto &newVaryings = MakeVaryings(type, count, arraySize);
    varyings->insert(varyings->end(), newVaryings.begin(), newVaryings.end());
}

// Test that a single varying can't overflow the packing.
TEST_P(VaryingPackingTest, OneVaryingLargerThanMax)
{
    ASSERT_FALSE(packVaryings(1, MakeVaryings(GL_FLOAT_MAT4, 1, 0)));
}

// This will overflow the available varying space.
TEST_P(VaryingPackingTest, MaxPlusOneVaryingVec3)
{
    ASSERT_FALSE(packVaryings(kMaxVaryings, MakeVaryings(GL_FLOAT_VEC3, kMaxVaryings + 1, 0)));
}

// This will overflow the available varying space.
TEST_P(VaryingPackingTest, MaxPlusOneVaryingVec3Array)
{
    ASSERT_FALSE(packVaryings(kMaxVaryings, MakeVaryings(GL_FLOAT_VEC3, kMaxVaryings / 2 + 1, 2)));
}

// This will overflow the available varying space.
TEST_P(VaryingPackingTest, MaxVaryingVec3AndOneVec2)
{
    std::vector<sh::ShaderVariable> varyings = MakeVaryings(GL_FLOAT_VEC3, kMaxVaryings, 0);
    AddVaryings(&varyings, GL_FLOAT_VEC2, 1, 0);
    ASSERT_FALSE(packVaryings(kMaxVaryings, varyings));
}

// This should work since two vec2s are packed in a single register.
TEST_P(VaryingPackingTest, MaxPlusOneVaryingVec2)
{
    ASSERT_TRUE(packVaryings(kMaxVaryings, MakeVaryings(GL_FLOAT_VEC2, kMaxVaryings + 1, 0)));
}

// Same for this one as above.
TEST_P(VaryingPackingTest, TwiceMaxVaryingVec2)
{
    ASSERT_TRUE(packVaryings(kMaxVaryings, MakeVaryings(GL_FLOAT_VEC2, kMaxVaryings * 2, 0)));
}

// This should not work since it overflows available varying space.
TEST_P(VaryingPackingTest, TooManyVaryingVec2)
{
    ASSERT_FALSE(packVaryings(kMaxVaryings, MakeVaryings(GL_FLOAT_VEC2, kMaxVaryings * 2 + 1, 0)));
}

// This should work according to the example GL packing rules - the float varyings are slotted
// into the end of the vec3 varying arrays.
TEST_P(VaryingPackingTest, MaxVaryingVec3ArrayAndFloatArrays)
{
    std::vector<sh::ShaderVariable> varyings = MakeVaryings(GL_FLOAT_VEC3, kMaxVaryings / 2, 2);
    AddVaryings(&varyings, GL_FLOAT, kMaxVaryings / 2, 2);
    ASSERT_TRUE(packVaryings(kMaxVaryings, varyings));
}

// This should not work - it has one too many float arrays.
TEST_P(VaryingPackingTest, MaxVaryingVec3ArrayAndMaxPlusOneFloatArray)
{
    std::vector<sh::ShaderVariable> varyings = MakeVaryings(GL_FLOAT_VEC3, kMaxVaryings / 2, 2);
    AddVaryings(&varyings, GL_FLOAT, kMaxVaryings / 2 + 1, 2);
    ASSERT_FALSE(packVaryings(kMaxVaryings, varyings));
}

// WebGL should fail to pack max+1 vec2 arrays, unlike our more relaxed packing.
TEST_P(VaryingPackingTest, MaxPlusOneMat2VaryingsFailsWebGL)
{
    auto varyings = MakeVaryings(GL_FLOAT_MAT2, kMaxVaryings / 2 + 1, 0);
    ASSERT_FALSE(packVaryingsStrict(kMaxVaryings, varyings));
}

// Each struct field consumes an entire register row, so kMaxVaryings float fields fit while
// kMaxVaryings + 1 float fields overflow available rows.
TEST_P(VaryingPackingTest, StructWithFloatFields)
{
    // GLSL equivalent (N = kMaxVaryings):
    //   struct S {
    //       mediump float m0;
    //       ...
    //       mediump float m{N-1};
    //   };
    //   flat out S v;  // matched with "flat in S v;" in the fragment shader
    std::vector<sh::ShaderVariable> fitVaryings = {MakeStructVarying("v", GL_FLOAT, kMaxVaryings)};
    ASSERT_TRUE(packVaryings(kMaxVaryings, fitVaryings));
    ASSERT_TRUE(packVaryingsStrict(kMaxVaryings, fitVaryings));

    // GLSL equivalent (N = kMaxVaryings):
    //   struct S {
    //       mediump float m0;
    //       ...
    //       mediump float m{N};
    //   };
    //   flat out S v;  // matched with "flat in S v;" in the fragment shader
    std::vector<sh::ShaderVariable> overflowVaryings = {
        MakeStructVarying("v", GL_FLOAT, kMaxVaryings + 1)};
    ASSERT_FALSE(packVaryings(kMaxVaryings, overflowVaryings));
    ASSERT_FALSE(packVaryingsStrict(kMaxVaryings, overflowVaryings));
}

// Capturing any field of a struct varying via transform feedback requires packing all fields of
// the struct, each occupying a full register row.
TEST_P(VaryingPackingTest, StructTransformFeedbackVaryings)
{
    // GLSL equivalent (N = kMaxVaryings):
    //   struct S {
    //       mediump float m0;
    //       ...
    //       mediump float m{N-1};
    //   };
    //   flat out S v;
    // Transform feedback varyings: {"v.m0"} (or {"v.m0", "v.m1"} when N > 1)
    std::vector<sh::ShaderVariable> fitVaryings = {MakeStructVarying("v", GL_FLOAT, kMaxVaryings)};
    std::vector<std::string> tfVaryings         = {"v.m0"};
    if (kMaxVaryings > 1)
    {
        tfVaryings.push_back("v.m1");
    }
    ASSERT_TRUE(testVaryingPackingWithTransformFeedback(kMaxVaryings, PackMode::WEBGL_STRICT,
                                                        fitVaryings, tfVaryings));

    // GLSL equivalent (N = kMaxVaryings):
    //   struct S {
    //       mediump float m0;
    //       ...
    //       mediump float m{N};
    //   };
    //   flat out S v;
    // Transform feedback varyings: {"v.m0", "v.m1"}
    std::vector<sh::ShaderVariable> overflowVaryings = {
        MakeStructVarying("v", GL_FLOAT, kMaxVaryings + 1)};
    ASSERT_FALSE(testVaryingPackingWithTransformFeedback(kMaxVaryings, PackMode::WEBGL_STRICT,
                                                         overflowVaryings, {"v.m0", "v.m1"}));
    ASSERT_FALSE(testVaryingPackingWithTransformFeedback(kMaxVaryings, PackMode::ANGLE_RELAXED,
                                                         overflowVaryings, {"v.m0", "v.m1"}));

    // An uncaptured, unmatched struct varying remains inactive and does not fail packing.
    // GLSL equivalent (N = kMaxVaryings):
    //   struct S {
    //       mediump float m0;
    //       ...
    //       mediump float m{N};
    //   };
    //   flat out mediump float float0;
    //   flat out S v;
    // Transform feedback varyings: {"float0"}
    std::vector<sh::ShaderVariable> inactiveAndActiveVaryings = MakeVaryings(GL_FLOAT, 1, 0);
    std::string activeVaryingName                             = inactiveAndActiveVaryings[0].name;
    inactiveAndActiveVaryings.push_back(MakeStructVarying("v", GL_FLOAT, kMaxVaryings + 1));
    ASSERT_TRUE(testVaryingPackingWithTransformFeedback(
        kMaxVaryings, PackMode::WEBGL_STRICT, inactiveAndActiveVaryings, {activeVaryingName}));

    // Capturing fields from two struct varyings that share the same struct type name ("S") must
    // pack all fields of both structs.
    if (kMaxVaryings > 1)
    {
        // GLSL equivalent (M = kMaxVaryings / 2):
        //   struct S {
        //       mediump float m0;
        //       ...
        //       mediump float m{M-1};
        //   };
        //   flat out S v1;
        //   flat out S v2;
        // Transform feedback varyings: {"v1.m0", "v2.m0"}
        std::vector<sh::ShaderVariable> twoSameTypeStructs = {
            MakeStructVarying("v1", GL_FLOAT, kMaxVaryings / 2),
            MakeStructVarying("v2", GL_FLOAT, kMaxVaryings / 2)};
        ASSERT_TRUE(testVaryingPackingWithTransformFeedback(
            kMaxVaryings, PackMode::WEBGL_STRICT, twoSameTypeStructs, {"v1.m0", "v2.m0"}));

        // GLSL equivalent (M = kMaxVaryings / 2):
        //   struct S1 {
        //       mediump float m0;
        //       ...
        //       mediump float m{M};
        //   };
        //   struct S2 {
        //       mediump float m0;
        //       ...
        //       mediump float m{M-1};
        //   };
        //   flat out S1 v1;
        //   flat out S2 v2;
        // Transform feedback varyings: {"v1.m0", "v2.m0"}
        std::vector<sh::ShaderVariable> twoSameTypeStructsOverflow = {
            MakeStructVarying("v1", GL_FLOAT, kMaxVaryings / 2 + 1),
            MakeStructVarying("v2", GL_FLOAT, kMaxVaryings / 2)};
        ASSERT_FALSE(testVaryingPackingWithTransformFeedback(
            kMaxVaryings, PackMode::WEBGL_STRICT, twoSameTypeStructsOverflow, {"v1.m0", "v2.m0"}));
    }

    // Capturing an array element inside a shader I/O block ("Block.arr[0]") or a block containing
    // an array of nested structs must pack all fields and elements of the block.
    if (kMaxVaryings >= 4)
    {
        // GLSL equivalent (N = kMaxVaryings):
        //   out Block {
        //       flat mediump float arr[2];
        //       flat mediump float m1;
        //       ...
        //       flat mediump float m{N-2};
        //   } blockInst;
        // Transform feedback varyings: {"Block.arr[0]"}
        // Total register rows = 2 (for arr[2]) + (N - 2) = N rows (fits in kMaxVaryings).
        sh::ShaderVariable ioBlock = MakeStructVarying("blockInst", GL_FLOAT, kMaxVaryings - 1);
        ioBlock.isShaderIOBlock    = true;
        ioBlock.structOrBlockName  = "Block";
        for (sh::ShaderVariable &field : ioBlock.fields)
        {
            field.isShaderIOBlock = true;
        }
        ioBlock.fields[0].name       = "arr";
        ioBlock.fields[0].mappedName = "arr";
        ioBlock.fields[0].arraySizes.push_back(2u);
        ASSERT_TRUE(testVaryingPackingWithTransformFeedback(kMaxVaryings, PackMode::WEBGL_STRICT,
                                                            {ioBlock}, {"Block.arr[0]"}));

        // Increasing arr to arr[3] requires 3 + (N - 2) = N + 1 rows (overflows kMaxVaryings):
        //   out Block {
        //       flat mediump float arr[3];
        //       flat mediump float m1;
        //       ...
        //       flat mediump float m{N-2};
        //   } blockInst;
        // Transform feedback varyings: {"Block.arr[0]"}
        ioBlock.fields[0].arraySizes[0] = 3u;
        ASSERT_FALSE(testVaryingPackingWithTransformFeedback(kMaxVaryings, PackMode::WEBGL_STRICT,
                                                             {ioBlock}, {"Block.arr[0]"}));

        // GLSL equivalent (M = kMaxVaryings / 2):
        //   struct S {
        //       mediump float m0;
        //       mediump float m1;
        //   };
        //   out Block {
        //       flat S s[M];
        //       flat mediump float f;
        //   } blockInst;
        // Transform feedback varyings: {"Block.f"}
        // Total register rows = M * 2 (for s[M]) + 1 (for f) = kMaxVaryings + 1 rows (overflows).
        sh::ShaderVariable ioBlockWithStructArray;
        ioBlockWithStructArray.type              = GL_NONE;
        ioBlockWithStructArray.precision         = GL_NONE;
        ioBlockWithStructArray.name              = "blockInst";
        ioBlockWithStructArray.mappedName        = "blockInst";
        ioBlockWithStructArray.structOrBlockName = "Block";
        ioBlockWithStructArray.isShaderIOBlock   = true;
        ioBlockWithStructArray.staticUse         = true;
        ioBlockWithStructArray.active            = true;
        ioBlockWithStructArray.interpolation     = sh::INTERPOLATION_FLAT;

        sh::ShaderVariable nestedStructArray = MakeStructVarying("s", GL_FLOAT, 2);
        nestedStructArray.isShaderIOBlock    = true;
        nestedStructArray.arraySizes.push_back(static_cast<unsigned int>(kMaxVaryings / 2));
        for (sh::ShaderVariable &nestedField : nestedStructArray.fields)
        {
            nestedField.isShaderIOBlock = true;
        }
        ioBlockWithStructArray.fields.push_back(nestedStructArray);

        sh::ShaderVariable capturedField = MakeStructVarying("tmp", GL_FLOAT, 1).fields[0];
        capturedField.name               = "f";
        capturedField.mappedName         = "f";
        capturedField.isShaderIOBlock    = true;
        ioBlockWithStructArray.fields.push_back(capturedField);

        ASSERT_FALSE(testVaryingPackingWithTransformFeedback(
            kMaxVaryings, PackMode::WEBGL_STRICT, {ioBlockWithStructArray}, {"Block.f"}));
    }
}

// Makes separate tests for different values of kMaxVaryings.
INSTANTIATE_TEST_SUITE_P(, VaryingPackingTest, ::testing::Values(1, 4, 8));

}  // anonymous namespace
