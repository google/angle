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
            '<(angle_path)/tests/angle_tests/ANGLETest.cpp',
            '<(angle_path)/tests/angle_tests/ANGLETest.h',
            '<(angle_path)/tests/angle_tests/BindTexImageTest.cpp',
            '<(angle_path)/tests/angle_tests/BlendMinMaxTest.cpp',
            '<(angle_path)/tests/angle_tests/BlitFramebufferANGLETest.cpp',
            '<(angle_path)/tests/angle_tests/BufferDataTest.cpp',
            '<(angle_path)/tests/angle_tests/ClearTest.cpp',
            '<(angle_path)/tests/angle_tests/CompressedTextureTest.cpp',
            '<(angle_path)/tests/angle_tests/CubeMapTextureTest.cpp',
            '<(angle_path)/tests/angle_tests/DepthStencilFormatsTest.cpp',
            '<(angle_path)/tests/angle_tests/DrawBuffersTest.cpp',
            '<(angle_path)/tests/angle_tests/FramebufferFormatsTest.cpp',
            '<(angle_path)/tests/angle_tests/GLSLTest.cpp',
            '<(angle_path)/tests/angle_tests/IncompleteTextureTest.cpp',
            '<(angle_path)/tests/angle_tests/IndexedPointsTest.cpp',
            '<(angle_path)/tests/angle_tests/InstancingTest.cpp',
            '<(angle_path)/tests/angle_tests/LineLoopTest.cpp',
            '<(angle_path)/tests/angle_tests/MaxTextureSizeTest.cpp',
            '<(angle_path)/tests/angle_tests/MipmapTest.cpp',
            '<(angle_path)/tests/angle_tests/media/pixel.inl',
            '<(angle_path)/tests/angle_tests/OcclusionQueriesTest.cpp',
            '<(angle_path)/tests/angle_tests/PBOExtensionTest.cpp',
            '<(angle_path)/tests/angle_tests/PointSpritesTest.cpp',
            '<(angle_path)/tests/angle_tests/ProgramBinaryTest.cpp',
            '<(angle_path)/tests/angle_tests/ReadPixelsTest.cpp',
            '<(angle_path)/tests/angle_tests/RendererTest.cpp',
            '<(angle_path)/tests/angle_tests/SimpleOperationTest.cpp',
            '<(angle_path)/tests/angle_tests/SRGBTextureTest.cpp',
            '<(angle_path)/tests/angle_tests/SwizzleTest.cpp',
            '<(angle_path)/tests/angle_tests/TextureTest.cpp',
            '<(angle_path)/tests/angle_tests/TransformFeedbackTest.cpp',
            '<(angle_path)/tests/angle_tests/UniformTest.cpp',
            '<(angle_path)/tests/angle_tests/UnpackAlignmentTest.cpp',
            '<(angle_path)/tests/angle_tests/UnpackRowLength.cpp',
            '<(angle_path)/tests/angle_tests/VertexAttributeTest.cpp',
            '<(angle_path)/tests/angle_tests/ViewportTest.cpp',
            '<(angle_path)/tests/standalone_tests/EGLThreadTest.cpp',
            '<(angle_path)/tests/standalone_tests/EGLQueryContextTest.cpp',
        ],
    },
    'dependencies':
    [
        '<(angle_path)/src/angle.gyp:libANGLE',
        '<(angle_path)/src/angle.gyp:libEGL',
        '<(angle_path)/src/angle.gyp:libGLESv2',
        '<(angle_path)/tests/tests.gyp:angle_test_support',
        '<(angle_path)/util/util.gyp:angle_util',
    ],
    'include_dirs':
    [
        '../include',
        'angle_tests',
    ],
    'includes':
    [
        # TODO(kbr): move these to angle_unittests.gypi.
        'angle_implementation_unit_tests/angle_implementation_unit_tests.gypi',
    ],
    'sources':
    [
        '<@(angle_end2end_tests_sources)',
    ],
}
