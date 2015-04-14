# Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This .gypi describes all of the sources and dependencies to build a
# unified "angle_end2end_tests" target, which contains all of the
# tests that exercise the ANGLE implementation. It requires a parent
# target to include this gypi in an executable target containing a
# gtest harness in a main.cpp.

{
    'variables':
    {
        # This file list will be shared with the GN build.
        'angle_end2end_tests_sources':
        [
            '<(angle_path)/src/tests/end2end_tests/ANGLETest.cpp',
            '<(angle_path)/src/tests/end2end_tests/ANGLETest.h',
            '<(angle_path)/src/tests/end2end_tests/BlendMinMaxTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/BlitFramebufferANGLETest.cpp',
            '<(angle_path)/src/tests/end2end_tests/BufferDataTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/ClearTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/CompressedTextureTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/CubeMapTextureTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/DepthStencilFormatsTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/DrawBuffersTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/FenceSyncTests.cpp',
            '<(angle_path)/src/tests/end2end_tests/FramebufferFormatsTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/FramebufferRenderMipmapTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/GLSLTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/IncompleteTextureTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/IndexedPointsTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/InstancingTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/LineLoopTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/MaxTextureSizeTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/MipmapTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/media/pixel.inl',
            '<(angle_path)/src/tests/end2end_tests/OcclusionQueriesTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/PBOExtensionTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/PbufferTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/PointSpritesTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/ProgramBinaryTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/ReadPixelsTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/RendererTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/SimpleOperationTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/SRGBTextureTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/SwizzleTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/TextureTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/TransformFeedbackTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/UniformBufferTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/UniformTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/UnpackAlignmentTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/UnpackRowLength.cpp',
            '<(angle_path)/src/tests/end2end_tests/VertexAttributeTest.cpp',
            '<(angle_path)/src/tests/end2end_tests/ViewportTest.cpp',
            '<(angle_path)/src/tests/standalone_tests/EGLThreadTest.cpp',
            '<(angle_path)/src/tests/standalone_tests/EGLQueryContextTest.cpp',
        ],
    },
    'dependencies':
    [
        '<(angle_path)/src/angle.gyp:libEGL',
        '<(angle_path)/src/angle.gyp:libGLESv2',
        '<(angle_path)/src/tests/tests.gyp:angle_test_support',
        '<(angle_path)/util/util.gyp:angle_util',
    ],
    'include_dirs':
    [
        '<(angle_path)/include',
        'end2end_tests',
    ],
    'sources':
    [
        '<@(angle_end2end_tests_sources)',
    ],
}
