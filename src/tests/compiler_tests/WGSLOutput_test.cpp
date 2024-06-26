//
// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// WGSLOutput_test.cpp:
//   Tests for corect WGSL translations.
//

#include <regex>
#include "GLSLANG/ShaderLang.h"
#include "angle_gl.h"
#include "gtest/gtest.h"
#include "tests/test_utils/compiler_test.h"

using namespace sh;

class WGSLVertexOutputTest : public MatchOutputCodeTest
{
  public:
    WGSLVertexOutputTest() : MatchOutputCodeTest(GL_VERTEX_SHADER, SH_WGSL_OUTPUT)
    {
        ShCompileOptions defaultCompileOptions = {};
        defaultCompileOptions.validateAST      = true;
        setDefaultCompileOptions(defaultCompileOptions);
    }
};

class WGSLOutputTest : public MatchOutputCodeTest
{
  public:
    WGSLOutputTest() : MatchOutputCodeTest(GL_FRAGMENT_SHADER, SH_WGSL_OUTPUT)
    {
        ShCompileOptions defaultCompileOptions = {};
        defaultCompileOptions.validateAST      = true;
        setDefaultCompileOptions(defaultCompileOptions);
    }
};

TEST_F(WGSLOutputTest, BasicTranslation)
{
    const std::string &shaderString =
        R"(#version 310 es
        precision highp float;

        out vec4 outColor;

        struct Foo {
            float x;
            float y;
            vec3 multiArray[2][3];
            mat3 aMatrix;
        };

        vec4 doFoo(Foo foo, float zw);

        vec4 doFoo(Foo foo, float zw)
        {
            // foo.x = foo.y;
            return vec4(foo.x, foo.y, zw, zw);
        }

        Foo returnFoo(Foo foo) {
          return foo;
        }

        float returnFloat(float x) {
          return x;
        }

        float takeArgs(vec2 x, float y) {
          return y;
        }

        void main()
        {
            Foo foo;
            // Struct field accesses.
            foo.x = 2.0;
            foo.y = 2.0;
            // Complicated constUnion should be emitted correctly.
            foo.multiArray = vec3[][](
              vec3[](
                vec3(1.0, 2.0, 3.0),
                vec3(1.0, 2.0, 3.0),
                vec3(1.0, 2.0, 3.0)),
              vec3[](
                vec3(4.0, 5.0, 6.0),
                vec3(4.0, 5.0, 6.0),
                vec3(4.0, 5.0, 6.0)
              )
            );
            int arrIndex = 1;
            // Access an array index with a constant index.
            float f = foo.multiArray[0][1].x;
            // Access an array index with a non-const index, should clamp by default.
            float f2 = foo.multiArray[0][arrIndex].x;
            gl_FragDepth = f + f2;
            doFoo(returnFoo(foo), returnFloat(3.0));
            takeArgs(vec2(1.0, 2.0), foo.x);
            returnFloat(doFoo(foo, 7.0 + 9.0).x);
        })";
    const std::string &outputString =
        R"(_uoutColor : vec4<f32>;

struct _uFoo
{
  _ux : f32,
  _uy : f32,
  _umultiArray : array<array<vec3<f32>, 3>, 2>,
  _uaMatrix : mat3x3<f32>,
};

fn _udoFoo(_ufoo : _uFoo, _uzw : f32) -> vec4<f32>;

fn _udoFoo(_ufoo : _uFoo, _uzw : f32) -> vec4<f32>
{
  ;
}

fn _ureturnFoo(_ufoo : _uFoo) -> _uFoo
{
  ;
}

fn _ureturnFloat(_ux : f32) -> f32
{
  ;
}

fn _utakeArgs(_ux : vec2<f32>, _uy : f32) -> f32
{
  ;
}

fn _umain()
{
  _ufoo : _uFoo;
  ((_ufoo)._ux) = (2.0f);
  ((_ufoo)._uy) = (2.0f);
  ((_ufoo)._umultiArray) = (array<array<vec3<f32>, 3>, 2>(array<vec3<f32>, 3>(vec3<f32>(1.0f, 2.0f, 3.0f), vec3<f32>(1.0f, 2.0f, 3.0f), vec3<f32>(1.0f, 2.0f, 3.0f)), array<vec3<f32>, 3>(vec3<f32>(4.0f, 5.0f, 6.0f), vec3<f32>(4.0f, 5.0f, 6.0f), vec3<f32>(4.0f, 5.0f, 6.0f))));
  _uarrIndex : i32 = (1i);
  _uf : f32 = (((((_ufoo)._umultiArray)[0i])[1i]).x);
  _uf2 : f32 = (((((_ufoo)._umultiArray)[0i])[clamp((_uarrIndex), 0, 2)]).x);
  (gl_FragDepth) = ((_uf) + (_uf2));
  _udoFoo(_ureturnFoo(_ufoo), _ureturnFloat(3.0f));
  _utakeArgs(vec2<f32>(1.0f, 2.0f), (_ufoo)._ux);
  _ureturnFloat((_udoFoo(_ufoo, 16.0f)).x);
}
)";
    compile(shaderString);
    EXPECT_TRUE(foundInCode(outputString.c_str()));
}
