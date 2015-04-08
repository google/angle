# Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This .gypi describes all of the sources and dependencies to build a
# unified "angle_unittests" target, which contains all of ANGLE's
# tests that don't require a fully functional ANGLE in order to run
# (compiler tests, preprocessor tests, etc.). It requires a parent
# target to include this gypi in an executable target containing a
# gtest harness in a main.cpp.

{
    'variables':
    {
        # This file list will be shared with the GN build.
        'angle_unittests_sources':
        [
            '<(angle_path)/src/common/Optional_unittest.cpp',
            '<(angle_path)/src/libANGLE/Config_unittest.cpp',
            '<(angle_path)/src/libANGLE/Fence_unittest.cpp',
            '<(angle_path)/src/libANGLE/HandleAllocator_unittest.cpp',
            '<(angle_path)/src/libANGLE/ImageIndexIterator_unittest.cpp',
            '<(angle_path)/src/libANGLE/ResourceManager_unittest.cpp',
            '<(angle_path)/src/libANGLE/Surface_unittest.cpp',
            '<(angle_path)/src/libANGLE/TransformFeedback_unittest.cpp',
            '<(angle_path)/src/tests/angle_unittests_utils.h',
            '<(angle_path)/src/tests/compiler_tests/API_test.cpp',
            '<(angle_path)/src/tests/compiler_tests/CollectVariables_test.cpp',
            '<(angle_path)/src/tests/compiler_tests/ConstantFolding_test.cpp',
            '<(angle_path)/src/tests/compiler_tests/DebugShaderPrecision_test.cpp',
            '<(angle_path)/src/tests/compiler_tests/ExpressionLimit_test.cpp',
            '<(angle_path)/src/tests/compiler_tests/MalformedShader_test.cpp',
            '<(angle_path)/src/tests/compiler_tests/ShaderExtension_test.cpp',
            '<(angle_path)/src/tests/compiler_tests/NV_draw_buffers_test.cpp',
            '<(angle_path)/src/tests/compiler_tests/ShaderVariable_test.cpp',
            '<(angle_path)/src/tests/compiler_tests/TypeTracking_test.cpp',
            '<(angle_path)/src/tests/preprocessor_tests/char_test.cpp',
            '<(angle_path)/src/tests/preprocessor_tests/comment_test.cpp',
            '<(angle_path)/src/tests/preprocessor_tests/define_test.cpp',
            '<(angle_path)/src/tests/preprocessor_tests/error_test.cpp',
            '<(angle_path)/src/tests/preprocessor_tests/extension_test.cpp',
            '<(angle_path)/src/tests/preprocessor_tests/identifier_test.cpp',
            '<(angle_path)/src/tests/preprocessor_tests/if_test.cpp',
            '<(angle_path)/src/tests/preprocessor_tests/input_test.cpp',
            '<(angle_path)/src/tests/preprocessor_tests/location_test.cpp',
            '<(angle_path)/src/tests/preprocessor_tests/MockDiagnostics.h',
            '<(angle_path)/src/tests/preprocessor_tests/MockDirectiveHandler.h',
            '<(angle_path)/src/tests/preprocessor_tests/number_test.cpp',
            '<(angle_path)/src/tests/preprocessor_tests/operator_test.cpp',
            '<(angle_path)/src/tests/preprocessor_tests/pragma_test.cpp',
            '<(angle_path)/src/tests/preprocessor_tests/PreprocessorTest.cpp',
            '<(angle_path)/src/tests/preprocessor_tests/PreprocessorTest.h',
            '<(angle_path)/src/tests/preprocessor_tests/space_test.cpp',
            '<(angle_path)/src/tests/preprocessor_tests/token_test.cpp',
            '<(angle_path)/src/tests/preprocessor_tests/version_test.cpp',
        ],
    },
    'dependencies':
    [
        '<(angle_path)/src/angle.gyp:libANGLE',
        '<(angle_path)/src/angle.gyp:preprocessor',
        '<(angle_path)/src/angle.gyp:translator_static',
        '<(angle_path)/src/tests/tests.gyp:angle_test_support',
    ],
    'include_dirs':
    [
        '<(angle_path)/include',
        '<(angle_path)/src',
        '<(angle_path)/src/compiler/preprocessor',
        '<(angle_path)/src/tests',
    ],
    'sources':
    [
        '<@(angle_unittests_sources)',
    ],
    'conditions':
    [
        ['angle_build_winrt==1',
        {
            'sources':
            [
                '<(angle_path)/src/libANGLE/renderer/d3d/d3d11/winrt/CoreWindowNativeWindow_unittest.cpp',
                '<(angle_path)/src/libANGLE/renderer/d3d/d3d11/winrt/SwapChainPanelNativeWindow_unittest.cpp',
            ],
            'defines':
            [
                'ANGLE_ENABLE_D3D11',
            ],
            'msvs_settings':
            {
                'VCLinkerTool':
                {
                    'AdditionalDependencies':
                    [
                        'runtimeobject.lib',
                    ],
                },
            },
        }],
    ],
}
