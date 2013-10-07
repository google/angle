# Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
    'conditions':
    [
        ['OS=="win"',
        {
            'targets':
            [
                {
                    'target_name': 'libEGL',
                    'type': 'shared_library',
                    'dependencies': [ 'libGLESv2' ],
                    'include_dirs':
                    [
                        '.',
                        '../include',
                        'libGLESv2',
                    ],
                    'sources': [ '<!@(python enumerate_files.py common libEGL -types *.cpp *.h *.def libEGL.rc)' ],
                    'msvs_disabled_warnings': [ 4267 ],
                    'msvs_settings':
                    {
                        'VCLinkerTool':
                        {
                            'AdditionalDependencies':
                            [
                                'd3d9.lib',
                            ],
                        },
                    },
                },
            ],
        },
        ],
    ],
}
