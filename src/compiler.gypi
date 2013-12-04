# Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'targets':
    [
        {
            'target_name': 'preprocessor',
            'type': 'static_library',
            'include_dirs': [ ],
            'sources': [ '<!@(python enumerate_files.py compiler/preprocessor -types *.cpp *.h)' ],
            'msvs_disabled_warnings': [ 4267 ],
        },

        {
            'target_name': 'translator',
            'type': '<(component)',
            'dependencies': [ 'preprocessor' ],
            'include_dirs':
            [
                '.',
                '../include',
            ],
            'defines':
            [
                'ANGLE_TRANSLATOR_IMPLEMENTATION',
            ],
            'sources': [ '<!@(python enumerate_files.py compiler/translator third_party/compiler common -types *.cpp *.h *.y *.l )', ],
            'conditions':
            [
                ['OS=="win"',
                    {
                        'msvs_disabled_warnings': [ 4267 ],
                        'sources/': [ [ 'exclude', 'compiler/translator/ossource_posix.cpp' ], ],
                    },
                    { # else: posix
                        'sources/': [ [ 'exclude', 'compiler/translator/ossource_win.cpp' ], ],
                    }
                ],
            ],
            'msvs_settings':
            {
                'VCLibrarianTool':
                {
                    'AdditionalOptions': ['/ignore:4221']
                },
            },
        },

        {
            'target_name': 'translator_static',
            'type': 'static_library',
            'dependencies': [ 'preprocessor' ],
            'include_dirs':
            [
                '.',
                '../include',
            ],
            'defines':
            [
                'ANGLE_TRANSLATOR_STATIC',
            ],
            'direct_dependent_settings':
            {
                'defines':
                [
                    'ANGLE_TRANSLATOR_STATIC',
                ],
            },
            'sources': [ '<!@(python enumerate_files.py compiler/translator third_party/compiler common -types *.cpp *.h *.y *.l )', ],
            'conditions':
            [
                ['OS=="win"',
                    {
                        'msvs_disabled_warnings': [ 4267 ],
                        'sources/': [ [ 'exclude', 'compiler/translator/ossource_posix.cpp' ], ],
                    },
                    { # else: posix
                        'sources/': [ [ 'exclude', 'compiler/translator/ossource_win.cpp' ], ],
                    }
                ],
            ],
            'msvs_settings':
            {
               'VCLibrarianTool':
                {
                    'AdditionalOptions': ['/ignore:4221']
                },
            },
        },
    ],
}
