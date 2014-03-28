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
            'sources': [ '<!@(python <(angle_build_scripts_path)/enumerate_files.py compiler/preprocessor -types *.cpp *.h)' ],
            'msvs_disabled_warnings': [ 4267 ],
        },

        {
            'target_name': 'translator',

            # TODO(jmadill): https://code.google.com/p/angleproject/issues/detail?id=569 component build
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
            'sources':
            [
                '<!@(python <(angle_build_scripts_path)/enumerate_files.py \
                     -dirs compiler/translator third_party/compiler common ../include \
                     -types *.cpp *.h *.y *.l)',
            ],
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
