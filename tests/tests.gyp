# Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'variables':
    {
        'angle_build_conformance_tests%': '0',
        'angle_build_deqp_tests%': '0',
    },
    'targets':
    [
        {
            'target_name': 'gtest',
            'type': 'static_library',
            'includes': [ '../build/common_defines.gypi', ],
            'include_dirs':
            [
                'third_party/googletest',
                'third_party/googletest/include',
            ],
            'sources':
            [
                'third_party/googletest/src/gtest-all.cc',
            ],
            'defines':
            [
                '_VARIADIC_MAX=10',
            ],
            'direct_dependent_settings':
            {
                'defines':
                [
                    '_VARIADIC_MAX=10',
                ],
            },
        },

        {
            'target_name': 'gmock',
            'type': 'static_library',
            'includes': [ '../build/common_defines.gypi', ],
            'include_dirs':
            [
                'third_party/googlemock',
                'third_party/googlemock/include',
                'third_party/googletest/include',
            ],
            'sources':
            [
                'third_party/googlemock/src/gmock-all.cc',
            ],
            'defines':
            [
                '_VARIADIC_MAX=10',
            ],
            'direct_dependent_settings':
            {
                'defines':
                [
                    '_VARIADIC_MAX=10',
                ],
            },
        },

        {
            'target_name': 'preprocessor_tests',
            'type': 'executable',
            'dependencies':
            [
                '../src/angle.gyp:preprocessor',
                'gtest',
                'gmock',
            ],
            'include_dirs':
            [
                '../src/compiler/preprocessor',
                'third_party/googletest/include',
                'third_party/googlemock/include',
            ],
            'includes':
            [
                '../build/common_defines.gypi',
                'preprocessor_tests/preprocessor_tests.gypi',
            ],
            'sources':
            [
                'preprocessor_tests/preprocessor_test_main.cpp',
            ],
        },

        {
            'target_name': 'compiler_tests',
            'type': 'executable',
            'dependencies':
            [
                '../src/angle.gyp:translator_static',
                'gtest',
            ],
            'include_dirs':
            [
                '../include',
                '../src',
                'third_party/googletest/include',
            ],
            'includes':
            [
                '../build/common_defines.gypi',
                'compiler_tests/compiler_tests.gypi',
            ],
            'sources':
            [
                'compiler_tests/compiler_test_main.cpp',
            ],
        },

        {
            'target_name': 'angle_implementation_unit_tests',
            'type': 'executable',
            'dependencies':
            [
                '../src/angle.gyp:libGLESv2_static',
                'gtest',
                'gmock',
            ],
            'include_dirs':
            [
                '../include',
                '../src',
                'third_party/googletest/include',
                'third_party/googlemock/include',
            ],
            'includes':
            [
                '../build/common_defines.gypi',
                'angle_implementation_unit_tests/angle_implementation_unit_tests.gypi',
            ],
            'sources':
            [
                'angle_implementation_unit_tests/angle_implementation_unit_tests_main.cpp',
            ],
        },
    ],

    'conditions':
    [
        ['OS=="win"',
        {
            'targets':
            [
                {
                    'target_name': 'angle_tests',
                    'type': 'executable',
                    'includes': [ '../build/common_defines.gypi', ],
                    'dependencies':
                    [
                        '../src/angle.gyp:libGLESv2',
                        '../src/angle.gyp:libEGL',
                        'gtest',
                        '../util/util.gyp:angle_util',
                    ],
                    'include_dirs':
                    [
                        '../include',
                        'angle_tests',
                        'third_party/googletest/include',
                    ],
                    'sources':
                    [
                        '<!@(python <(angle_path)/enumerate_files.py angle_tests -types *.cpp *.h *.inl)'
                    ],
                },
                {
                    'target_name': 'standalone_tests',
                    'type': 'executable',
                    'includes': [ '../build/common_defines.gypi', ],
                    'dependencies':
                    [
                        'gtest',
                        'gmock',
                    ],
                    'include_dirs':
                    [
                        '../include',
                        'angle_tests',
                        'third_party/googletest/include',
                        'third_party/googlemock/include',
                    ],
                    'sources':
                    [
                        '<!@(python <(angle_path)/enumerate_files.py standalone_tests -types *.cpp *.h)'
                    ],
                },
                {
                    'target_name': 'angle_perf_tests',
                    'type': 'executable',
                    'includes': [ '../build/common_defines.gypi', ],
                    'dependencies':
                    [
                        '../src/angle.gyp:libGLESv2',
                        '../src/angle.gyp:libEGL',
                        'gtest',
                        '../util/util.gyp:angle_util',
                    ],
                    'include_dirs':
                    [
                        '../include',
                        'third_party/googletest/include',
                    ],
                    'sources':
                    [
                        'perf_tests/BufferSubData.cpp',
                        'perf_tests/BufferSubData.h',
                        'perf_tests/SimpleBenchmark.cpp',
                        'perf_tests/SimpleBenchmark.h',
                        'perf_tests/SimpleBenchmarks.cpp',
                        'perf_tests/TexSubImage.cpp',
                        'perf_tests/TexSubImage.h',
                    ],
                },
            ],
            'conditions':
            [
                ['angle_build_conformance_tests',
                {
                    'variables':
                    {
                        'gles_conformance_tests_output_dir': '<(SHARED_INTERMEDIATE_DIR)/conformance_tests',
                        'gles_conformance_tests_input_dir': 'third_party/gles_conformance_tests/conform/GTF_ES/glsl/GTF',
                        'gles_conformance_tests_generator_script': 'gles_conformance_tests/generate_gles_conformance_tests.py',
                    },
                    'targets':
                    [
                        {
                            'target_name': 'gles2_conformance_tests',
                            'type': 'executable',
                            'includes': [ '../build/common_defines.gypi', ],
                            'dependencies':
                            [
                                '../src/angle.gyp:libGLESv2',
                                '../src/angle.gyp:libEGL',
                                'gtest',
                                'third_party/gles_conformance_tests/conform/GTF_ES/glsl/GTF/es_cts.gyp:es_cts_test_data',
                                'third_party/gles_conformance_tests/conform/GTF_ES/glsl/GTF/es_cts.gyp:es2_cts',
                            ],
                            'variables':
                            {
                                'gles2_conformance_tests_input_file': '<(gles_conformance_tests_input_dir)/mustpass_es20.run',
                                'gles2_conformance_tests_generated_file': '<(gles_conformance_tests_output_dir)/generated_gles2_conformance_tests.cpp',
                            },
                            'sources':
                            [
                                '<!@(python <(angle_path)/enumerate_files.py gles_conformance_tests -types *.cpp *.h *.inl)',
                                '<(gles2_conformance_tests_generated_file)',
                            ],
                            'include_dirs':
                            [
                                '../include',
                                'gles_conformance_tests',
                                'third_party/googletest/include',
                            ],
                            'defines':
                            [
                                'CONFORMANCE_TESTS_TYPE=CONFORMANCE_TESTS_ES2',
                            ],
                            'actions':
                            [
                                {
                                    'action_name': 'generate_gles2_conformance_tests',
                                    'message': 'Generating ES2 conformance tests...',
                                    'msvs_cygwin_shell': 0,
                                    'inputs':
                                    [
                                        '<(gles_conformance_tests_generator_script)',
                                        '<(gles2_conformance_tests_input_file)',
                                    ],
                                    'outputs':
                                    [
                                        '<(gles2_conformance_tests_generated_file)',
                                    ],
                                    'action':
                                    [
                                        'python',
                                        '<(gles_conformance_tests_generator_script)',
                                        '<(gles2_conformance_tests_input_file)',
                                        '<(gles_conformance_tests_input_dir)',
                                        '<(gles2_conformance_tests_generated_file)',
                                    ],
                                },
                            ],
                        },
                        {
                            'target_name': 'gles3_conformance_tests',
                            'type': 'executable',
                            'includes': [ '../build/common_defines.gypi', ],
                            'dependencies':
                            [
                                '../src/angle.gyp:libGLESv2',
                                '../src/angle.gyp:libEGL',
                                'gtest',
                                'third_party/gles_conformance_tests/conform/GTF_ES/glsl/GTF/es_cts.gyp:es_cts_test_data',
                                'third_party/gles_conformance_tests/conform/GTF_ES/glsl/GTF/es_cts.gyp:es3_cts',
                            ],
                            'variables':
                            {
                                'gles3_conformance_tests_input_file': '<(gles_conformance_tests_input_dir)/mustpass_es30.run',
                                'gles3_conformance_tests_generated_file': '<(gles_conformance_tests_output_dir)/generated_gles3_conformance_tests.cpp',
                            },
                            'sources':
                            [
                                '<!@(python <(angle_path)/enumerate_files.py gles_conformance_tests -types *.cpp *.h *.inl)',
                                '<(gles3_conformance_tests_generated_file)',
                            ],
                            'include_dirs':
                            [
                                '../include',
                                'gles_conformance_tests',
                                'third_party/googletest/include',
                            ],
                            'defines':
                            [
                                'CONFORMANCE_TESTS_TYPE=CONFORMANCE_TESTS_ES3',
                            ],
                            'msvs_settings':
                            {
                                'VCCLCompilerTool':
                                {
                                    # MSVS has trouble compiling this due to the obj files becoming too large.
                                    'AdditionalOptions': [ '/bigobj' ],
                                },
                            },
                            'actions':
                            [
                                {
                                    'action_name': 'generate_gles3_conformance_tests',
                                    'message': 'Generating ES3 conformance tests...',
                                    'msvs_cygwin_shell': 0,
                                    'inputs':
                                    [
                                        '<(gles_conformance_tests_generator_script)',
                                        '<(gles3_conformance_tests_input_file)',
                                    ],
                                    'outputs':
                                    [
                                        '<(gles3_conformance_tests_generated_file)',
                                    ],
                                    'action':
                                    [
                                        'python',
                                        '<(gles_conformance_tests_generator_script)',
                                        '<(gles3_conformance_tests_input_file)',
                                        '<(gles_conformance_tests_input_dir)',
                                        '<(gles3_conformance_tests_generated_file)',
                                    ],
                                },
                            ],
                        },
                    ],
                }],
                ['angle_build_deqp_tests',
                {
                    'targets':
                    [
                        {
                            'target_name': 'deqp_tests',
                            'type': 'executable',
                            'includes': [ '../build/common_defines.gypi', ],
                            'dependencies':
                            [
                                '../src/angle.gyp:libGLESv2',
                                '../src/angle.gyp:libEGL',
                                'gtest',
                                'third_party/deqp/src/deqp/modules/gles3/gles3.gyp:deqp-gles3',
                                'third_party/deqp/src/deqp/framework/platform/platform.gyp:tcutil-platform',
                            ],
                            'include_dirs':
                            [
                                '../include',
                                'third_party/googletest/include',
                                'deqp_tests',
                            ],
                            'variables':
                            {
                                'deqp_tests_output_dir': '<(SHARED_INTERMEDIATE_DIR)/deqp_tests',
                                'deqp_tests_input_file': 'deqp_tests/deqp_tests.txt',
                                'deqp_tests_generated_file': '<(deqp_tests_output_dir)/generated_deqp_tests.cpp',
                            },
                            'sources':
                            [
                                '<!@(python <(angle_path)/enumerate_files.py deqp_tests -types *.cpp *.h *.inl)',
                                '<(deqp_tests_generated_file)',
                            ],
                            'actions':
                            [
                                {
                                    'action_name': 'generate_deqp_tests',
                                    'message': 'Generating dEQP tests...',
                                    'msvs_cygwin_shell': 0,
                                    'variables':
                                    {
                                        'deqp_tests_generator_script': 'deqp_tests/generate_deqp_tests.py',
                                    },
                                    'inputs':
                                    [
                                        '<(deqp_tests_generator_script)',
                                        '<(deqp_tests_input_file)',
                                    ],
                                    'outputs':
                                    [
                                        '<(deqp_tests_generated_file)',
                                    ],
                                    'action':
                                    [
                                        'python',
                                        '<(deqp_tests_generator_script)',
                                        '<(deqp_tests_input_file)',
                                        '<(deqp_tests_generated_file)',
                                    ],
                                },
                            ],
                        },
                    ],
                }],
            ],
        }],
    ],
}
