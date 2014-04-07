# Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
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
        },

        {
            'target_name': 'compiler_tests',
            'type': 'executable',
            'dependencies':
            [
                '../src/angle.gyp:translator',
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
                'compiler_tests/compiler_tests.gypi',
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
            ],
        }],
    ],
}
