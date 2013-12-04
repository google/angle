# Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'targets':
    [
        {
            'target_name': 'gtest',
            'type': 'static_library',
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
            'sources':
            [
                '<!@(python enumerate_files.py preprocessor_tests -types *.cpp *.h)'
            ],
        },

        {
            'target_name': 'compiler_tests',
            'type': 'executable',
            'dependencies':
            [
                '../src/angle.gyp:translator_static',
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
            'sources':
            [
                '<!@(python enumerate_files.py compiler_tests -types *.cpp *.h)'
            ],
        },
    ],
}
